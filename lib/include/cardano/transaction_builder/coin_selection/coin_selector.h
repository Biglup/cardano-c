/**
 * \file coin_selector.h
 *
 * \author angel.castillo
 * \date   Oct 14, 2024
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_COIN_SELECTOR_H
#define BIGLUP_LABS_INCLUDE_CARDANO_COIN_SELECTOR_H

/* INCLUDES ******************************************************************/

#include <cardano/error.h>
#include <cardano/export.h>

#include <cardano/transaction_builder/coin_selection/coin_selector_impl.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Coin Selector Interface for Cardano.
 *
 * The `cardano_coin_selector_t` structure provides an interface for performing coin selection operations.
 * The purpose of this structure is to enable efficient and optimized selection of UTXOs (Unspent Transaction Outputs)
 * that can fulfill a specific transaction's required value while minimizing the transaction size and fees.
 *
 * Coin selection is a process where inputs are chosen to cover a target value,
 * while potentially minimizing dust (small UTXOs), reducing the change size, or optimizing for other factors.
 * The strategy and algorithm used in the coin selection can be customized and configured to suit various
 * transaction requirements.
 *
 * This structure includes the necessary context and configuration for selecting UTXOs from a pool of
 * available outputs, along with additional functionality such as filtering and prioritizing UTXOs.
 *
 * Coin selection algorithms implemented within this structure might include:
 * - Largest First: Selecting the largest UTXOs first to reduce the number of inputs.
 * - Random Improve: A method where random UTXOs are selected with a focus on minimizing dust.
 * - Custom strategies, as defined by implementers.
 *
 * \remark The caller must manage the life of the \c selected_utxo and \c remaining_utxo lists byt calling
 * \ref cardano_utxo_list_unref when they are no longer needed.
 *
 * \see CIP-0002: https://cips.cardano.org/cip/CIP-0002
 *
 * Example usage:
 * \code{.c}
 * cardano_coin_selector_t* coin_selector = ...;  // Initialize the coin selector
 * cardano_utxo_list_t* available_utxo = ...;     // Available UTXOs
 * cardano_value_t* target_value = ...;           // The target value to cover
 * cardano_utxo_list_t* selected_utxo = NULL;
 * cardano_utxo_list_t* remaining_utxo = NULL;
 *
 * cardano_error_t result = cardano_coin_selector_select(
 *     coin_selector,
 *     available_utxo,
 *     target_value,
 *     &selected_utxo,
 *     &remaining_utxo);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Coin selection succeeded
 * }
 *
 * // Free the selected and remaining UTXO lists when done
 * cardano_utxo_list_unref(&selected_utxo);
 * cardano_utxo_list_unref(&remaining_utxo);
 * \endcode
 */
typedef struct cardano_coin_selector_t cardano_coin_selector_t;

/**
 * \brief Creates a new `cardano_coin_selector_t` object using the provided implementation.
 *
 * This function initializes a new \ref cardano_coin_selector_t object by wrapping the given
 * \ref cardano_coin_selector_impl_t implementation. The newly created coin_selector object manages
 * the lifecycle of the underlying implementation and provides an interface for interacting
 * with the Cardano blockchain functionalities.
 *
 * \param[in]  impl     The coin_selector implementation containing function pointers and context.
 * \param[out] coin_selector A pointer to store the address of the newly created coin_selector object.
 *                      This should be a valid pointer to a \ref cardano_coin_selector_t* variable.
 *
 * \returns A \ref cardano_error_t indicating success or failure of the operation.
 *
 * Usage Example:
 * \code{.c}
 * // Assume 'impl' is a valid cardano_coin_selector_impl_t initialized elsewhere.
 * cardano_coin_selector_t* coin_selector = NULL;
 * cardano_error_t result = cardano_coin_selector_new(impl, &coin_selector);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the coin_selector
 * }
 * else
 * {
 *   printf("Error: %s\n", cardano_error_to_string(result));
 * }
 *
 * // When done with the coin_selector
 * cardano_coin_selector_unref(&coin_selector);
 * \endcode
 *
 * \note After successfully creating a \ref cardano_coin_selector_t object, you are responsible for
 *       managing its lifecycle. Ensure that you call \ref cardano_coin_selector_unref when the
 *       coin_selector is no longer needed to release resources and prevent memory leaks.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_coin_selector_new(cardano_coin_selector_impl_t impl, cardano_coin_selector_t** coin_selector);

/**
 * \brief Retrieves the name of the coin_selector implementation.
 *
 * This function returns a constant string representing the name of the coin_selector implementation.
 * The name can be used for logging, debugging, or informational purposes to identify which
 * coin_selector implementation is being used.
 *
 * \param[in] coin_selector Pointer to the \ref cardano_coin_selector_t object.
 *
 * \returns A constant character pointer to the coin_selector's name string.
 *          The returned string is owned by the coin_selector and must not be modified or freed by the caller.
 *          If the \p coin_selector is NULL or invalid, the function may return NULL.
 *
 * Usage Example:
 * \code{.c}
 * const char* coin_selector_name = cardano_coin_selector_get_name(coin_selector);
 *
 * if (coin_selector_name)
 * {
 *   printf("Using coin_selector: %s\n", coin_selector_name);
 * }
 * else
 * {
 *   printf("Failed to retrieve coin_selector name.\n");
 * }
 * \endcode
 *
 * \note The returned string remains valid as long as the \ref cardano_coin_selector_t object is valid.
 *       Do not attempt to modify or free the returned string.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_coin_selector_get_name(const cardano_coin_selector_t* coin_selector);

/**
 * \brief Selects UTXOs to satisfy the target value using a coin selection strategy.
 *
 * This function performs coin selection using the provided \ref cardano_coin_selector_t object, selecting UTXOs from the
 * available UTXO set to meet the specified target value. It returns the selected UTXOs and any remaining UTXOs that were
 * not chosen in separate output parameters.
 *
 * \param[in] coin_selector A pointer to the \ref cardano_coin_selector_t object, which defines the coin selection strategy.
 * \param[in] pre_selected_utxo An optional set of pre-selected UTXOs (can be NULL) to be used as part of the selection.
 * \param[in] available_utxo A list of available UTXOs from which the coin selection will be made.
 * \param[in] target The target value to be satisfied by the coin selection (in lovelace or multi-asset values).
 * \param[out] selection A pointer to a UTXO list where the selected UTXOs will be stored.
 *                       The caller is responsible for releasing the memory of this list when done.
 * \param[out] remaining_utxo A pointer to a UTXO list where the remaining, unselected UTXOs will be stored.
 *                            The caller is responsible for releasing the memory of this list when done.
 *
 * \return \ref CARDANO_SUCCESS if the coin selection succeeded, or an appropriate error code indicating failure.
 *
 * \note Both `selection` and `remaining_utxo` must be properly freed by the caller after use.
 *       The function ensures that UTXOs from the available pool are used optimally to meet the target value,
 *       following the coin selection strategy provided by the \ref cardano_coin_selector_t object.
 *
 * Usage Example:
 * \code{.c}
 * cardano_coin_selector_t* coin_selector = ...; // The coin selection strategy
 * cardano_utxo_list_t* available_utxo = ...;    // List of available UTXOs
 * cardano_value_t* target_value = ...;          // Target value in lovelace or multi-asset format
 * cardano_utxo_list_t* selected_utxo = NULL;
 * cardano_utxo_list_t* remaining_utxo = NULL;
 *
 * cardano_error_t result = cardano_coin_selector_select(
 *     coin_selector, pre_selected_utxo, available_utxo, target_value, &selected_utxo, &remaining_utxo);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Successfully selected UTXOs
 *   // selected_utxo contains the selected UTXOs
 *   // remaining_utxo contains the unselected UTXOs
 * }
 *
 * // Clean up UTXO lists after use
 * cardano_utxo_list_unref(&selected_utxo);
 * cardano_utxo_list_unref(&remaining_utxo);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_coin_selector_select(
  cardano_coin_selector_t* coin_selector,
  cardano_utxo_list_t*     pre_selected_utxo,
  cardano_utxo_list_t*     available_utxo,
  cardano_value_t*         target,
  cardano_utxo_list_t**    selection,
  cardano_utxo_list_t**    remaining_utxo);

/**
 * \brief Decrements the reference count of a cardano_coin_selector_t object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_coin_selector_t object
 * by decreasing its reference count. When the reference count reaches zero, the coin_selector is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] coin_selector A pointer to the pointer of the coin_selector object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_coin_selector_t* coin_selector = cardano_coin_selector_new(major, minor);
 *
 * // Perform operations with the coin_selector...
 *
 * cardano_coin_selector_unref(&coin_selector);
 * // At this point, coin_selector is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_coin_selector_unref, the pointer to the \ref cardano_coin_selector_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_coin_selector_unref(cardano_coin_selector_t** coin_selector);

/**
 * \brief Increases the reference count of the cardano_coin_selector_t object.
 *
 * This function is used to manually increment the reference count of an cardano_coin_selector_t
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_coin_selector_unref.
 *
 * \param coin_selector A pointer to the cardano_coin_selector_t object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming coin_selector is a previously created coin_selector object
 *
 * cardano_coin_selector_ref(coin_selector);
 *
 * // Now coin_selector can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_coin_selector_ref there is a corresponding
 * call to \ref cardano_coin_selector_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_coin_selector_ref(cardano_coin_selector_t* coin_selector);

/**
 * \brief Retrieves the current reference count of the cardano_coin_selector_t object.
 *
 * This function returns the number of active references to an cardano_coin_selector_t object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_coin_selector_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param coin_selector A pointer to the cardano_coin_selector_t object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified cardano_coin_selector_t object. If the object
 * is properly managed (i.e., every \ref cardano_coin_selector_ref call is matched with a
 * \ref cardano_coin_selector_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming coin_selector is a previously created coin_selector object
 *
 * size_t ref_count = cardano_coin_selector_refcount(coin_selector);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_coin_selector_refcount(const cardano_coin_selector_t* coin_selector);

/**
 * \brief Sets the last error message for a given cardano_coin_selector_t object.
 *
 * Records an error message in the coin_selector's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] coin_selector A pointer to the \ref cardano_coin_selector_t instance whose last error message is
 *                       to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the coin_selector's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_coin_selector_set_last_error(
  cardano_coin_selector_t* coin_selector,
  const char*              message);

/**
 * \brief Retrieves the last error message recorded for a specific coin_selector.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_coin_selector_set_last_error for the given
 * coin_selector. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] coin_selector A pointer to the \ref cardano_coin_selector_t instance whose last error
 *                   message is to be retrieved. If the coin_selector is NULL, the function
 *                   returns a generic error message indicating the null coin_selector.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified coin_selector. If the coin_selector is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_coin_selector_set_last_error for the same coin_selector, or until
 *       the coin_selector is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_coin_selector_get_last_error(const cardano_coin_selector_t* coin_selector);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_COIN_SELECTOR_H