/**
 * \file treasury_withdrawals_action.h
 *
 * \author angel.castillo
 * \date   Aug 14, 2024
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

#ifndef TREASURY_WITHDRAWALS_ACTION_H
#define TREASURY_WITHDRAWALS_ACTION_H

/* INCLUDES ******************************************************************/

#include <cardano/cbor/cbor_reader.h>
#include <cardano/cbor/cbor_writer.h>
#include <cardano/common/withdrawal_map.h>
#include <cardano/crypto/blake2b_hash.h>
#include <cardano/error.h>
#include <cardano/export.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Withdraws funds from the treasury.
 */
typedef struct cardano_treasury_withdrawals_action_t cardano_treasury_withdrawals_action_t;

/**
 * \brief Creates and initializes a new instance of the Treasury Withdrawals Action.
 *
 * This function allocates and initializes a new instance of a \ref cardano_treasury_withdrawals_action_t object,
 * which represents an action to withdraw funds from the Cardano treasury.
 *
 * **Guardrails Script Hash:**
 * The `policy_hash` parameter represents the hash of the guardrails script (also known as the governance action policy script).
 * The guardrails script is Plutus script that acts as a safeguard by imposing additional constraints on certain types
 * of governance actions, such as protocol parameter updates and treasury withdrawals. When proposing a treasury withdrawal,
 * you must provide its hash to reference it. This ensures that the proposal is validated against the guardrails script during the transaction processing.
 *
 * You can obtain the guardrails script hash using the `cardano-cli`:
 * \code{.sh}
 * cardano-cli hash script --script-file guardrails-script.plutus
 * \endcode
 *
 * Example output:
 * \code{.sh}
 * fa24fb305126805cf2164c161d852a0e7330cf988f1fe558cf7d4a64
 * \endcode
 *
 * \param[in] withdrawals A pointer to a \ref cardano_withdrawal_map_t object representing the set of withdrawals.
 *                        Each withdrawal consists of a reward address and the amount to withdraw.
 * \param[in] policy_hash An optional pointer to a \ref cardano_blake2b_hash_t object representing the hash of the guardrails script.
 * \param[out] treasury_withdrawals_action On successful initialization, this will point to a newly created
 *             \ref cardano_treasury_withdrawals_action_t object. This object represents a "strong reference"
 *             to the treasury withdrawals action, meaning that it is fully initialized and ready for use.
 *             The caller is responsible for managing the lifecycle of this object,
 *             specifically, once the treasury withdrawals action is no longer needed, the caller must release it
 *             by calling \ref cardano_treasury_withdrawals_action_unref.
 *
 * \return \c cardano_error_t indicating the outcome of the operation. Returns \c CARDANO_SUCCESS if the treasury withdrawals action was
 *         successfully created, or an appropriate error code indicating the failure reason.
 *
 * **Usage Example:**
 * \code{.c}
 * // Initialize the withdrawals map
 * cardano_withdrawal_map_t* withdrawals = cardano_withdrawal_map_new();
 * // Add a withdrawal to the map
 * cardano_reward_address_t* reward_address = cardano_reward_address_from_bech32("stake1u9...");
 * cardano_withdrawal_map_add(withdrawals, reward_address, 50000000000); // Withdraw 500 ADA
 *
 * // Obtain the guardrails script hash (if required)
 * cardano_blake2b_hash_t* policy_hash = cardano_blake2b_hash_from_hex("fa24fb305126805cf2164c161d852a0e7330cf988f1fe558cf7d4a64");
 *
 * cardano_treasury_withdrawals_action_t* treasury_withdrawals_action = NULL;
 * cardano_error_t result = cardano_treasury_withdrawals_action_new(withdrawals, policy_hash, &treasury_withdrawals_action);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the treasury withdrawals action
 *   // For example, add it to the transaction builder
 *   cardano_tx_builder_add_treasury_withdrawals_action(tx_builder, treasury_withdrawals_action);
 *
 *   // Free resources when done
 *   cardano_treasury_withdrawals_action_unref(&treasury_withdrawals_action);
 * }
 * else
 * {
 *   printf("Failed to create the treasury withdrawals action: %s\n", cardano_error_to_string(result));
 * }
 *
 * // Cleanup
 * cardano_withdrawal_map_unref(&withdrawals);
 * cardano_reward_address_unref(&reward_address);
 * cardano_blake2b_hash_unref(&policy_hash);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_treasury_withdrawals_action_new(
  cardano_withdrawal_map_t*               withdrawals,
  cardano_blake2b_hash_t*                 policy_hash,
  cardano_treasury_withdrawals_action_t** treasury_withdrawals_action);

/**
 * \brief Creates a \ref cardano_treasury_withdrawals_action_t from a CBOR reader.
 *
 * This function parses CBOR data using a provided \ref cardano_cbor_reader_t and constructs a \ref cardano_treasury_withdrawals_action_t object.
 * It assumes that the CBOR reader is set up correctly and that the CBOR data corresponds to the structure expected for a treasury_withdrawals_action.
 *
 * \param[in] reader A pointer to an initialized \ref cardano_cbor_reader_t that is ready to read the CBOR-encoded data.
 * \param[out] treasury_withdrawals_action A pointer to a pointer of \ref cardano_treasury_withdrawals_action_t that will be set to the address
 *                        of the newly created treasury_withdrawals_action object upon successful decoding.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the object was successfully created, or an appropriate error code if an error occurred.
 *
 * \note If the function fails, the last error can be retrieved by calling \ref cardano_cbor_reader_get_last_error with the reader.
 *       The caller is responsible for freeing the created \ref cardano_treasury_withdrawals_action_t object by calling
 *       \ref cardano_treasury_withdrawals_action_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_new(cbor_data, data_size);
 * cardano_treasury_withdrawals_action_t* treasury_withdrawals_action = NULL;
 *
 * cardano_error_t result = cardano_treasury_withdrawals_action_from_cbor(reader, &treasury_withdrawals_action);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the treasury_withdrawals_action
 *
 *   // Once done, ensure to clean up and release the treasury_withdrawals_action
 *   cardano_treasury_withdrawals_action_unref(&treasury_withdrawals_action);
 * }
 * else
 * {
 *   const char* error = cardano_cbor_reader_get_last_error(reader);
 *   printf("Failed to decode treasury_withdrawals_action: %s\n", error);
 * }
 *
 * cardano_cbor_reader_unref(&reader); // Cleanup the CBOR reader
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_treasury_withdrawals_action_from_cbor(cardano_cbor_reader_t* reader, cardano_treasury_withdrawals_action_t** treasury_withdrawals_action);

/**
 * \brief Serializes the certificate into CBOR format using a CBOR writer.
 *
 * This function serializes the given \ref cardano_treasury_withdrawals_action_t object using a \ref cardano_cbor_writer_t.
 *
 * \param[in] treasury_withdrawals_action A constant pointer to the \ref cardano_treasury_withdrawals_action_t object that is to be serialized.
 * \param[out] writer A pointer to a \ref cardano_cbor_writer_t where the CBOR serialized data will be written.
 *                    The writer must already be initialized and ready to accept the data.
 *
 * \return Returns \ref CARDANO_SUCCESS if the serialization is successful. If the \p treasury_withdrawals_action or \p writer
 *         is NULL, returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_treasury_withdrawals_action_t* treasury_withdrawals_action = ...;
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * if (writer)
 * {
 *   cardano_error_t result = cardano_treasury_withdrawals_action_to_cbor(treasury_withdrawals_action, writer);
 *
 *   if (result == CARDANO_SUCCESS)
 *   {
 *     // Use the writer's buffer containing the serialized data
 *   }
 *   else
 *   {
 *     const char* error_message = cardano_cbor_writer_get_last_error(writer);
 *     printf("Serialization failed: %s\n", error_message);
 *   }
 *
 *   cardano_cbor_writer_unref(&writer);
 * }
 *
 * cardano_treasury_withdrawals_action_unref(&treasury_withdrawals_action);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_treasury_withdrawals_action_to_cbor(
  const cardano_treasury_withdrawals_action_t* treasury_withdrawals_action,
  cardano_cbor_writer_t*                       writer);

/**
 * \brief Sets the withdrawals in the treasury withdrawals action.
 *
 * This function updates the withdrawals section of a \ref cardano_treasury_withdrawals_action_t object.
 * The withdrawals are represented by a \ref cardano_withdrawal_map_t object that details the specific withdrawals to be made from the treasury.
 *
 * \param[in,out] treasury_withdrawals_action A pointer to an initialized \ref cardano_treasury_withdrawals_action_t object to which the withdrawals will be set.
 * \param[in] withdrawals A pointer to an initialized \ref cardano_withdrawal_map_t object representing the new withdrawals.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the withdrawals were
 *         successfully set, or an appropriate error code indicating the failure reason, such as \ref CARDANO_ERROR_POINTER_IS_NULL if the
 *         treasury_withdrawals_action pointer is NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_treasury_withdrawals_action_t* treasury_withdrawals_action = ...; // Assume treasury_withdrawals_action is already initialized
 * cardano_withdrawal_map_t* withdrawals = cardano_withdrawal_map_new(...); // Assume withdrawals is already initialized
 *
 * cardano_error_t result = cardano_treasury_withdrawals_action_set_withdrawals(treasury_withdrawals_action, withdrawals);
 * if (result == CARDANO_SUCCESS)
 * {
 *   // The withdrawals are now set for the treasury withdrawals action
 * }
 * else
 * {
 *   printf("Failed to set the withdrawals.\n");
 * }
 *
 * // Cleanup the treasury_withdrawals_action and withdrawals after use
 * cardano_treasury_withdrawals_action_unref(&treasury_withdrawals_action);
 * cardano_withdrawal_map_unref(&withdrawals);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_treasury_withdrawals_action_set_withdrawals(cardano_treasury_withdrawals_action_t* treasury_withdrawals_action, cardano_withdrawal_map_t* withdrawals);

/**
 * \brief Retrieves the withdrawals from a treasury_withdrawals_action.
 *
 * This function retrieves the withdrawals from a given \ref cardano_treasury_withdrawals_action_t object. The withdrawals
 * are represented as a \ref cardano_withdrawal_map_t object, detailing the specific withdrawals planned from the treasury.
 *
 * \param[in] treasury_withdrawals_action A pointer to an initialized \ref cardano_treasury_withdrawals_action_t object from which the withdrawals are retrieved.
 *
 * \return A pointer to the retrieved \ref cardano_withdrawal_map_t object representing the withdrawals.
 *         This will be a new reference, and the caller is responsible for releasing it with \ref cardano_withdrawal_map_unref
 *         when it is no longer needed. If no withdrawals are set, NULL may be returned.
 *
 * Usage Example:
 * \code{.c}
 * cardano_treasury_withdrawals_action_t* treasury_withdrawals_action = ...; // Assume initialized
 * cardano_withdrawal_map_t* withdrawals = cardano_treasury_withdrawals_action_get_withdrawals(treasury_withdrawals_action);
 *
 * if (withdrawals != NULL)
 * {
 *   // Process the withdrawals
 *
 *   // Once done, ensure to clean up and release the withdrawals
 *   cardano_withdrawal_map_unref(&withdrawals);
 * }
 * else
 * {
 *   printf("ERROR: No withdrawals set for this treasury withdrawals action.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_withdrawal_map_t*
cardano_treasury_withdrawals_action_get_withdrawals(cardano_treasury_withdrawals_action_t* treasury_withdrawals_action);

/**
 * \brief Sets the policy hash in the treasury_withdrawals_action.
 *
 * This function updates the policy hash of a \ref cardano_treasury_withdrawals_action_t object.
 * The policy hash is a \ref cardano_blake2b_hash_t object that can optionally be set to modify the identification of the policy.
 * This parameter can be NULL if the policy hash is to be unset, effectively removing any previously set policy hash.
 *
 * \param[in,out] treasury_withdrawals_action A pointer to an initialized \ref cardano_treasury_withdrawals_action_t object to which the policy hash will be set.
 * \param[in] policy_hash A pointer to an initialized \ref cardano_blake2b_hash_t object representing the new policy hash. This parameter can be NULL if the policy hash is to be unset.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the policy hash was
 *         successfully set, or an appropriate error code indicating the failure reason, such as \ref CARDANO_ERROR_POINTER_IS_NULL if any of the
 *         input pointers are NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_treasury_withdrawals_action_t* treasury_withdrawals_action = ...; // Assume treasury_withdrawals_action is already initialized
 * cardano_blake2b_hash_t* policy_hash = cardano_blake2b_hash_new(...); // Optionally initialized
 *
 * cardano_error_t result = cardano_treasury_withdrawals_action_set_policy_hash(treasury_withdrawals_action, policy_hash);
 * if (result == CARDANO_SUCCESS)
 * {
 *   // The policy hash is now set for the treasury_withdrawals_action
 * }
 * else
 * {
 *   printf("Failed to set the policy hash.\n");
 * }
 *
 * // Clean up the treasury_withdrawals_action and optionally the policy hash after use
 * cardano_treasury_withdrawals_action_unref(&treasury_withdrawals_action);
 *
 * if (policy_hash)
 * {
 *   cardano_blake2b_hash_unref(&policy_hash);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_treasury_withdrawals_action_set_policy_hash(cardano_treasury_withdrawals_action_t* treasury_withdrawals_action, cardano_blake2b_hash_t* policy_hash);

/**
 * \brief Retrieves the policy hash from a treasury_withdrawals_action.
 *
 * This function retrieves the policy hash from a given \ref cardano_treasury_withdrawals_action_t object. The policy hash
 * is represented as a \ref cardano_blake2b_hash_t object.
 *
 * \param[in] treasury_withdrawals_action A pointer to an initialized \ref cardano_treasury_withdrawals_action_t object from which the policy hash is retrieved.
 *
 * \return A pointer to the retrieved \ref cardano_blake2b_hash_t object representing the policy hash.
 *         This will be a new reference, and the caller is responsible for releasing it with \ref cardano_blake2b_hash_unref
 *         when it is no longer needed. If the treasury_withdrawals_action does not have a policy hash set, NULL is returned.
 *
 * Usage Example:
 * \code{.c}
 * cardano_treasury_withdrawals_action_t* treasury_withdrawals_action = ...; // Assume initialized
 * cardano_blake2b_hash_t* policy_hash = cardano_treasury_withdrawals_action_get_policy_hash(treasury_withdrawals_action);
 *
 * if (policy_hash != NULL)
 * {
 *   printf("Policy Hash: %s\n", cardano_blake2b_hash_to_string(policy_hash));
 *   // Use the policy hash
 *
 *   // Once done, ensure to clean up and release the policy hash
 *   cardano_blake2b_hash_unref(&policy_hash);
 * }
 * else
 * {
 *   printf("No policy hash set for this parameter change action.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_blake2b_hash_t*
cardano_treasury_withdrawals_action_get_policy_hash(cardano_treasury_withdrawals_action_t* treasury_withdrawals_action);

/**
 * \brief Decrements the reference count of a cardano_treasury_withdrawals_action_t object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_treasury_withdrawals_action_t object
 * by decreasing its reference count. When the reference count reaches zero, the treasury_withdrawals_action is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] treasury_withdrawals_action A pointer to the pointer of the treasury_withdrawals_action object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_treasury_withdrawals_action_t* treasury_withdrawals_action = cardano_treasury_withdrawals_action_new(major, minor);
 *
 * // Perform operations with the treasury_withdrawals_action...
 *
 * cardano_treasury_withdrawals_action_unref(&treasury_withdrawals_action);
 * // At this point, treasury_withdrawals_action is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_treasury_withdrawals_action_unref, the pointer to the \ref cardano_treasury_withdrawals_action_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_treasury_withdrawals_action_unref(cardano_treasury_withdrawals_action_t** treasury_withdrawals_action);

/**
 * \brief Increases the reference count of the cardano_treasury_withdrawals_action_t object.
 *
 * This function is used to manually increment the reference count of an cardano_treasury_withdrawals_action_t
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_treasury_withdrawals_action_unref.
 *
 * \param treasury_withdrawals_action A pointer to the cardano_treasury_withdrawals_action_t object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming treasury_withdrawals_action is a previously created treasury_withdrawals_action object
 *
 * cardano_treasury_withdrawals_action_ref(treasury_withdrawals_action);
 *
 * // Now treasury_withdrawals_action can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_treasury_withdrawals_action_ref there is a corresponding
 * call to \ref cardano_treasury_withdrawals_action_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_treasury_withdrawals_action_ref(cardano_treasury_withdrawals_action_t* treasury_withdrawals_action);

/**
 * \brief Retrieves the current reference count of the cardano_treasury_withdrawals_action_t object.
 *
 * This function returns the number of active references to an cardano_treasury_withdrawals_action_t object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_treasury_withdrawals_action_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param treasury_withdrawals_action A pointer to the cardano_treasury_withdrawals_action_t object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified cardano_treasury_withdrawals_action_t object. If the object
 * is properly managed (i.e., every \ref cardano_treasury_withdrawals_action_ref call is matched with a
 * \ref cardano_treasury_withdrawals_action_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming treasury_withdrawals_action is a previously created treasury_withdrawals_action object
 *
 * size_t ref_count = cardano_treasury_withdrawals_action_refcount(treasury_withdrawals_action);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_treasury_withdrawals_action_refcount(const cardano_treasury_withdrawals_action_t* treasury_withdrawals_action);

/**
 * \brief Sets the last error message for a given cardano_treasury_withdrawals_action_t object.
 *
 * Records an error message in the treasury_withdrawals_action's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] treasury_withdrawals_action A pointer to the \ref cardano_treasury_withdrawals_action_t instance whose last error message is
 *                       to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the treasury_withdrawals_action's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_treasury_withdrawals_action_set_last_error(
  cardano_treasury_withdrawals_action_t* treasury_withdrawals_action,
  const char*                            message);

/**
 * \brief Retrieves the last error message recorded for a specific treasury_withdrawals_action.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_treasury_withdrawals_action_set_last_error for the given
 * treasury_withdrawals_action. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] treasury_withdrawals_action A pointer to the \ref cardano_treasury_withdrawals_action_t instance whose last error
 *                   message is to be retrieved. If the treasury_withdrawals_action is NULL, the function
 *                   returns a generic error message indicating the null treasury_withdrawals_action.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified treasury_withdrawals_action. If the treasury_withdrawals_action is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_treasury_withdrawals_action_set_last_error for the same treasury_withdrawals_action, or until
 *       the treasury_withdrawals_action is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_treasury_withdrawals_action_get_last_error(
  const cardano_treasury_withdrawals_action_t* treasury_withdrawals_action);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // TREASURY_WITHDRAWALS_ACTION_H