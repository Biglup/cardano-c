/**
 * \file secure_key_handler.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_SECURE_KEY_HANDLER_H
#define BIGLUP_LABS_INCLUDE_CARDANO_SECURE_KEY_HANDLER_H

/* INCLUDES ******************************************************************/

#include <cardano/error.h>
#include <cardano/export.h>

#include <cardano/key_handlers/account_derivation_path.h>
#include <cardano/key_handlers/secure_key_handler_impl.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Opaque type definition for the secure key handler.
 *
 * `cardano_secure_key_handler_t` is an opaque structure representing a secure key handler. This handler abstracts the management of
 * cryptographic key operations for both BIP32 (HD) and Ed25519 key types, ensuring that sensitive key material is managed securely.
 * The internal details of this structure are hidden from users of the API, who interact with it through
 * the provided API functions.
 */
typedef struct cardano_secure_key_handler_t cardano_secure_key_handler_t;

/**
 * \brief Creates a new `cardano_secure_key_handler_t` object using the provided implementation.
 *
 * This function initializes a new \ref cardano_secure_key_handler_t object by wrapping the given
 * \ref cardano_secure_key_handler_impl_t implementation. The newly created secure_key_handler object manages
 * the lifecycle of the underlying implementation and provides an interface for interacting
 * with cryptographic keys securely.
 *
 * \param[in]  impl     The secure_key_handler implementation containing function pointers and context.
 * \param[out] secure_key_handler A pointer to store the address of the newly created secure_key_handler object.
 *                      This should be a valid pointer to a \ref cardano_secure_key_handler_t* variable.
 *
 * \returns A \ref cardano_error_t indicating success or failure of the operation.
 *
 * Usage Example:
 * \code{.c}
 * // Assume 'impl' is a valid cardano_secure_key_handler_impl_t initialized elsewhere.
 * cardano_secure_key_handler_t* secure_key_handler = NULL;
 * cardano_error_t result = cardano_secure_key_handler_new(impl, &secure_key_handler);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the secure_key_handler
 * }
 * else
 * {
 *   printf("Error: %s\n", cardano_error_to_string(result));
 * }
 *
 * // When done with the secure_key_handler
 * cardano_secure_key_handler_unref(&secure_key_handler);
 * \endcode
 *
 * \note After successfully creating a \ref cardano_secure_key_handler_t object, you are responsible for
 *       managing its lifecycle. Ensure that you call \ref cardano_secure_key_handler_unref when the
 *       secure_key_handler is no longer needed to release resources and prevent memory leaks.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_secure_key_handler_new(cardano_secure_key_handler_impl_t impl, cardano_secure_key_handler_t** secure_key_handler);

/**
 * \brief Retrieves the name of the secure_key_handler implementation.
 *
 * This function returns a constant string representing the name of the secure_key_handler implementation.
 * The name can be used for logging, debugging, or informational purposes to identify which
 * secure_key_handler implementation is being used.
 *
 * \param[in] secure_key_handler Pointer to the \ref cardano_secure_key_handler_t object.
 *
 * \returns A constant character pointer to the secure_key_handler's name string.
 *          The returned string is owned by the secure_key_handler and must not be modified or freed by the caller.
 *          If the \p secure_key_handler is NULL or invalid, the function may return NULL.
 *
 * Usage Example:
 * \code{.c}
 * const char* secure_key_handler_name = cardano_secure_key_handler_get_name(secure_key_handler);
 *
 * if (secure_key_handler_name)
 * {
 *   printf("Using secure_key_handler: %s\n", secure_key_handler_name);
 * }
 * else
 * {
 *   printf("Failed to retrieve secure_key_handler name.\n");
 * }
 * \endcode
 *
 * \note The returned string remains valid as long as the \ref cardano_secure_key_handler_t object is valid.
 *       Do not attempt to modify or free the returned string.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_secure_key_handler_get_name(const cardano_secure_key_handler_t* secure_key_handler);

/**
 * \brief Signs a transaction using BIP32 Hierarchical Deterministic (HD) keys.
 *
 * This function uses the `cardano_secure_key_handler_t` to securely manage the cryptographic key operations needed to
 * sign a transaction using BIP32 (HD) keys. It derives the necessary private keys based on the provided `derivation_paths`
 * and signs the transaction `tx`, generating a set of verification key witnesses (`vkey_witness_set`).
 *
 * \param[in] secure_key_handler A pointer to the secure key handler managing the cryptographic key operations.
 * \param[in] tx The transaction object to be signed.
 * \param[in] derivation_paths An array of BIP32 derivation paths used to derive the private keys for signing the transaction.
 * \param[in] num_paths The number of derivation paths provided in the `derivation_paths` array.
 * \param[out] vkey_witness_set A pointer to the verification key witness set that will be populated with the generated signatures.
 *                         This set will contain the signatures and associated verification keys required for the transaction.
 *
 * \returns `cardano_error_t` indicating success or the type of error encountered during signing.
 *
 * \note The caller is responsible for managing and releasing the `vkey_witness_set` to avoid memory leaks.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_secure_key_handler_bip32_sign_transaction(
  cardano_secure_key_handler_t*    secure_key_handler,
  cardano_transaction_t*           tx,
  const cardano_derivation_path_t* derivation_paths,
  size_t                           num_paths,
  cardano_vkey_witness_set_t**     vkey_witness_set);

/**
 * \brief Retrieves the extended BIP32 account public key for a given derivation path.
 *
 * This function securely derives and retrieves the extended BIP32 account public key for the `cardano_secure_key_handler_t`. The extended
 * BIP32 public key includes both the Ed25519 public key and chain code, enabling further key derivations within the BIP32 standard.
 *
 * \param[in] secure_key_handler A pointer to the secure key handler managing the cryptographic key operations.
 * \param[in] derivation_path The BIP32 derivation path used to derive the extended account public key.
 * \param[out] bip32_public_key A pointer to the extended BIP32 public key that will be retrieved and returned.
 *                         The caller must manage the lifecycle of the returned `bip32_public_key` and release it when done.
 *
 * \returns `cardano_error_t` indicating success or the type of error encountered during key retrieval.
 *
 * \note The caller is responsible for managing the lifecycle of the `bip32_public_key` object, ensuring it is properly released
 *       to avoid memory leaks.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_secure_key_handler_bip32_get_extended_account_public_key(
  cardano_secure_key_handler_t*     secure_key_handler,
  cardano_account_derivation_path_t derivation_path,
  cardano_bip32_public_key_t**      bip32_public_key);

/**
 * \brief Signs a transaction using Ed25519 keys.
 *
 * This function securely signs the provided `tx` (transaction) using the Ed25519 keys managed by the `cardano_secure_key_handler_t`.
 * The resulting witness set, containing the verification keys and signatures, is returned in the `vkey_witness_set`.
 *
 * \param[in] secure_key_handler A pointer to the secure key handler managing the Ed25519 cryptographic operations.
 * \param[in] tx The transaction object to be signed.
 * \param[out] vkey_witness_set A pointer to the verification key witness set that will be populated with the generated signatures.
 *                         The caller must manage the lifecycle of the `vkey_witness_set` to avoid memory leaks.
 *
 * \returns `cardano_error_t` indicating success or the type of error encountered during signing.
 *
 * \note This function does not support BIP32 hierarchical deterministic keys. It is intended for use with Ed25519 keys only.
 * \note The caller is responsible for managing the lifecycle of the `vkey_witness_set` to ensure proper memory management.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_secure_key_handler_ed25519_sign_transaction(
  cardano_secure_key_handler_t* secure_key_handler,
  cardano_transaction_t*        tx,
  cardano_vkey_witness_set_t**  vkey_witness_set);

/**
 * \brief Retrieves the public key associated with the Ed25519 key.
 *
 * This function securely retrieves the Ed25519 public key from the `cardano_secure_key_handler_t`. The resulting public key
 * is returned in the `public_key` parameter.
 *
 * \param[in] secure_key_handler A pointer to the secure key handler managing the Ed25519 cryptographic operations.
 * \param[out] public_key A pointer to the location where the Ed25519 public key will be stored. The caller is responsible for managing
 *                   the lifecycle of this object and must release it appropriately to avoid memory leaks.
 *
 * \returns `cardano_error_t` indicating success or the type of error encountered during the retrieval.
 *
 * \note This function only handles Ed25519 public keys and does not support BIP32 keys.
 * \note The caller is responsible for managing the lifecycle of the `public_key` to ensure proper memory management.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_secure_key_handler_ed25519_get_public_key(
  cardano_secure_key_handler_t*  secure_key_handler,
  cardano_ed25519_public_key_t** public_key);

/**
 * \brief Decrements the reference count of a cardano_secure_key_handler_t object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_secure_key_handler_t object
 * by decreasing its reference count. When the reference count reaches zero, the secure_key_handler is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] secure_key_handler A pointer to the pointer of the secure_key_handler object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_secure_key_handler_t* secure_key_handler = cardano_secure_key_handler_new(major, minor);
 *
 * // Perform operations with the secure_key_handler...
 *
 * cardano_secure_key_handler_unref(&secure_key_handler);
 * // At this point, secure_key_handler is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_secure_key_handler_unref, the pointer to the \ref cardano_secure_key_handler_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_secure_key_handler_unref(cardano_secure_key_handler_t** secure_key_handler);

/**
 * \brief Increases the reference count of the cardano_secure_key_handler_t object.
 *
 * This function is used to manually increment the reference count of an cardano_secure_key_handler_t
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_secure_key_handler_unref.
 *
 * \param secure_key_handler A pointer to the cardano_secure_key_handler_t object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming secure_key_handler is a previously created secure_key_handler object
 *
 * cardano_secure_key_handler_ref(secure_key_handler);
 *
 * // Now secure_key_handler can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_secure_key_handler_ref there is a corresponding
 * call to \ref cardano_secure_key_handler_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_secure_key_handler_ref(cardano_secure_key_handler_t* secure_key_handler);

/**
 * \brief Serializes the state of a secure key handler.
 *
 * The `cardano_secure_key_handler_serialize` function serializes the internal state or key material associated
 * with a secure key handler into a buffer. This allows the key handler to be stored or transferred in a secure, serialized format.
 *
 * This function is expected to:
 * - Serialize the current state of the secure key handler into the provided `serialized_data` buffer.
 *
 * \param secure_key_handler A pointer to the secure key handler that manages cryptographic operations.
 * \param serialized_data A pointer to the buffer that will be populated with the serialized state of the key handler.
 *                        This buffer must be managed and freed by the caller to prevent memory leaks.
 *
 * \returns `cardano_error_t` indicating success or the type of error encountered during serialization.
 *
 * \note The caller is responsible for releasing the serialized buffer after use to avoid memory leaks.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_secure_key_handler_serialize(
  cardano_secure_key_handler_t* secure_key_handler,
  cardano_buffer_t**            serialized_data);

/**
 * \brief Retrieves the current reference count of the cardano_secure_key_handler_t object.
 *
 * This function returns the number of active references to an cardano_secure_key_handler_t object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_secure_key_handler_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param secure_key_handler A pointer to the cardano_secure_key_handler_t object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified cardano_secure_key_handler_t object. If the object
 * is properly managed (i.e., every \ref cardano_secure_key_handler_ref call is matched with a
 * \ref cardano_secure_key_handler_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming secure_key_handler is a previously created secure_key_handler object
 *
 * size_t ref_count = cardano_secure_key_handler_refcount(secure_key_handler);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_secure_key_handler_refcount(const cardano_secure_key_handler_t* secure_key_handler);

/**
 * \brief Sets the last error message for a given cardano_secure_key_handler_t object.
 *
 * Records an error message in the secure_key_handler's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] secure_key_handler A pointer to the \ref cardano_secure_key_handler_t instance whose last error message is
 *                       to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the secure_key_handler's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_secure_key_handler_set_last_error(
  cardano_secure_key_handler_t* secure_key_handler,
  const char*                   message);

/**
 * \brief Retrieves the last error message recorded for a specific secure_key_handler.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_secure_key_handler_set_last_error for the given
 * secure_key_handler. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] secure_key_handler A pointer to the \ref cardano_secure_key_handler_t instance whose last error
 *                   message is to be retrieved. If the secure_key_handler is NULL, the function
 *                   returns a generic error message indicating the null secure_key_handler.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified secure_key_handler. If the secure_key_handler is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_secure_key_handler_set_last_error for the same secure_key_handler, or until
 *       the secure_key_handler is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_secure_key_handler_get_last_error(const cardano_secure_key_handler_t* secure_key_handler);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_SECURE_KEY_HANDLER_H