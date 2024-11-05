/**
 * \file tx_evaluator.h
 *
 * \author angel.castillo
 * \date   Nov 05, 2024
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_TX_EVALUATOR_H
#define BIGLUP_LABS_INCLUDE_CARDANO_TX_EVALUATOR_H

/* INCLUDES ******************************************************************/

#include <cardano/error.h>
#include <cardano/export.h>

#include <cardano/transaction_builder/evaluation/tx_evaluator_impl.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Opaque structure for a transaction evaluator.
 *
 * The `cardano_tx_evaluator_t` structure serves as the public-facing handle for managing
 * the transaction evaluation process within the Cardano framework.
 */
typedef struct cardano_tx_evaluator_t cardano_tx_evaluator_t;

/**
 * \brief Creates a new transaction evaluator instance.
 *
 * This function initializes a new `cardano_tx_evaluator_t` instance using the provided implementation.
 * The `tx_evaluator` parameter will be set to point to the newly created evaluator, which can then
 * be used for evaluating transactions' execution units, considering additional UTXOs and redeemers.
 *
 * \param[in] impl A `cardano_tx_evaluator_impl_t` implementation that defines the specific evaluation strategy and operations.
 * \param[out] tx_evaluator A double pointer to the `cardano_tx_evaluator_t` instance, which will be allocated and set by this function.
 *
 * \return \ref CARDANO_SUCCESS if the evaluator was successfully created, or an appropriate error code if the operation failed.
 *
 * \note The caller is responsible for releasing the created `tx_evaluator` instance by using the appropriate unref function when done.
 *
 * Usage Example:
 * \code{.c}
 * cardano_tx_evaluator_t* evaluator = NULL;
 * cardano_tx_evaluator_impl_t impl = ...; // Assume this is set up with a valid implementation.
 * cardano_error_t result = cardano_tx_evaluator_new(impl, &evaluator);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *     // Evaluator successfully created, ready for use
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_tx_evaluator_new(cardano_tx_evaluator_impl_t impl, cardano_tx_evaluator_t** tx_evaluator);

/**
 * \brief Retrieves the name of the tx_evaluator implementation.
 *
 * This function returns a constant string representing the name of the tx_evaluator implementation.
 * The name can be used for logging, debugging, or informational purposes to identify which
 * tx_evaluator implementation is being used.
 *
 * \param[in] tx_evaluator Pointer to the \ref cardano_tx_evaluator_t object.
 *
 * \returns A constant character pointer to the tx_evaluator's name string.
 *          The returned string is owned by the tx_evaluator and must not be modified or freed by the caller.
 *          If the \p tx_evaluator is NULL or invalid, the function may return NULL.
 *
 * Usage Example:
 * \code{.c}
 * const char* tx_evaluator_name = cardano_tx_evaluator_get_name(tx_evaluator);
 *
 * if (tx_evaluator_name)
 * {
 *   printf("Using tx_evaluator: %s\n", tx_evaluator_name);
 * }
 * else
 * {
 *   printf("Failed to retrieve tx_evaluator name.\n");
 * }
 * \endcode
 *
 * \note The returned string remains valid as long as the \ref cardano_tx_evaluator_t object is valid.
 *       Do not attempt to modify or free the returned string.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_tx_evaluator_get_name(const cardano_tx_evaluator_t* tx_evaluator);

/**
 * \brief Evaluates the execution units required for a transaction.
 *
 * This function calculates the execution units needed for a given transaction (`tx`) by using the provided `tx_evaluator`.
 * Evaluation considers any additional UTXOs required for the transaction and assigns appropriate redeemers based on the evaluation.
 *
 * \param[in] tx_evaluator A pointer to the \ref cardano_tx_evaluator_t instance used to evaluate the transaction.
 * \param[in] tx A pointer to the \ref cardano_transaction_t object representing the transaction to evaluate.
 * \param[in] additional_utxos An optional list of additional UTXOs that may be needed for transaction evaluation.
 * \param[out] redeemers A double pointer to a \ref cardano_redeemer_list_t, which will be populated with the evaluated redeemers
 *                       that specify the execution units needed for the transaction.
 *
 * \return \ref CARDANO_SUCCESS if the transaction was successfully evaluated, or an appropriate error code indicating failure.
 *
 * \note The caller is responsible for managing and releasing memory for the redeemers list once it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_redeemer_list_t* redeemers = NULL;
 * cardano_error_t result = cardano_tx_evaluator_evaluate(tx_evaluator, tx, additional_utxos, &redeemers);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Redeemers were successfully evaluated and are now available in the redeemers list
 * }
 *
 * // Clean up when done
 * cardano_redeemer_list_unref(&redeemers);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_tx_evaluator_evaluate(
  cardano_tx_evaluator_t*   tx_evaluator,
  cardano_transaction_t*    tx,
  cardano_utxo_list_t*      additional_utxos,
  cardano_redeemer_list_t** redeemers);

/**
 * \brief Decrements the reference count of a cardano_tx_evaluator_t object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_tx_evaluator_t object
 * by decreasing its reference count. When the reference count reaches zero, the tx_evaluator is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] tx_evaluator A pointer to the pointer of the tx_evaluator object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_tx_evaluator_t* tx_evaluator = cardano_tx_evaluator_new(major, minor);
 *
 * // Perform operations with the tx_evaluator...
 *
 * cardano_tx_evaluator_unref(&tx_evaluator);
 * // At this point, tx_evaluator is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_tx_evaluator_unref, the pointer to the \ref cardano_tx_evaluator_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_tx_evaluator_unref(cardano_tx_evaluator_t** tx_evaluator);

/**
 * \brief Increases the reference count of the cardano_tx_evaluator_t object.
 *
 * This function is used to manually increment the reference count of an cardano_tx_evaluator_t
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_tx_evaluator_unref.
 *
 * \param tx_evaluator A pointer to the cardano_tx_evaluator_t object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming tx_evaluator is a previously created tx_evaluator object
 *
 * cardano_tx_evaluator_ref(tx_evaluator);
 *
 * // Now tx_evaluator can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_tx_evaluator_ref there is a corresponding
 * call to \ref cardano_tx_evaluator_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_tx_evaluator_ref(cardano_tx_evaluator_t* tx_evaluator);

/**
 * \brief Retrieves the current reference count of the cardano_tx_evaluator_t object.
 *
 * This function returns the number of active references to an cardano_tx_evaluator_t object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_tx_evaluator_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param tx_evaluator A pointer to the cardano_tx_evaluator_t object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified cardano_tx_evaluator_t object. If the object
 * is properly managed (i.e., every \ref cardano_tx_evaluator_ref call is matched with a
 * \ref cardano_tx_evaluator_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming tx_evaluator is a previously created tx_evaluator object
 *
 * size_t ref_count = cardano_tx_evaluator_refcount(tx_evaluator);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_tx_evaluator_refcount(const cardano_tx_evaluator_t* tx_evaluator);

/**
 * \brief Sets the last error message for a given cardano_tx_evaluator_t object.
 *
 * Records an error message in the tx_evaluator's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] tx_evaluator A pointer to the \ref cardano_tx_evaluator_t instance whose last error message is
 *                       to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the tx_evaluator's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_tx_evaluator_set_last_error(
  cardano_tx_evaluator_t* tx_evaluator,
  const char*             message);

/**
 * \brief Retrieves the last error message recorded for a specific tx_evaluator.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_tx_evaluator_set_last_error for the given
 * tx_evaluator. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] tx_evaluator A pointer to the \ref cardano_tx_evaluator_t instance whose last error
 *                   message is to be retrieved. If the tx_evaluator is NULL, the function
 *                   returns a generic error message indicating the null tx_evaluator.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified tx_evaluator. If the tx_evaluator is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_tx_evaluator_set_last_error for the same tx_evaluator, or until
 *       the tx_evaluator is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_tx_evaluator_get_last_error(const cardano_tx_evaluator_t* tx_evaluator);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_TX_EVALUATOR_H