/**
 * \file builder_outputs.h
 *
 * \author angel.castillo
 * \date   Jul 18, 2026
 *
 * Copyright 2026 Biglup Labs
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_BUILDER_OUTPUTS_H
#define BIGLUP_LABS_INCLUDE_CARDANO_BUILDER_OUTPUTS_H

/* INCLUDES ******************************************************************/

#include <cardano/address/address.h>
#include <cardano/common/datum.h>
#include <cardano/error.h>
#include <cardano/transaction_body/transaction_output.h>
#include <cardano/transaction_body/value.h>

#include "builder_state.h"

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Adds an output that sends the given amount of lovelace to an address.
 *
 * This function creates a transaction output paying \p amount lovelace to \p address and appends it
 * to the output list of the transaction body under construction.
 *
 * \param[in,out] state A pointer to the \ref cardano_builder_state_t tracking the transaction under
 *                      construction. This parameter must not be NULL.
 * \param[in] address A pointer to the \ref cardano_address_t receiving the lovelace. The output holds
 *                    a reference to it.
 * \param[in] amount The amount of lovelace to send.
 * \param[out] error_message A pointer that receives a static string describing the failure when the
 *                           function does not return \ref CARDANO_SUCCESS. It is left untouched on
 *                           success. This parameter must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the output was added, or an appropriate error code indicating the
 *         failure reason.
 */
cardano_error_t
cardano_builder_send_lovelace(
  cardano_builder_state_t* state,
  cardano_address_t*       address,
  uint64_t                 amount,
  const char**             error_message);

/**
 * \brief Adds an output that sends the given amount of lovelace to an address given as a string.
 *
 * This function parses the address string and delegates to \ref cardano_builder_send_lovelace.
 *
 * \param[in,out] state A pointer to the \ref cardano_builder_state_t tracking the transaction under
 *                      construction. This parameter must not be NULL.
 * \param[in] address A pointer to the character string with the destination address.
 * \param[in] address_size The size of the address string in bytes.
 * \param[in] amount The amount of lovelace to send.
 * \param[out] error_message A pointer that receives a static string describing the failure when the
 *                           function does not return \ref CARDANO_SUCCESS. It is left untouched on
 *                           success. This parameter must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the output was added, or an appropriate error code indicating the
 *         failure reason.
 */
cardano_error_t
cardano_builder_send_lovelace_ex(
  cardano_builder_state_t* state,
  const char*              address,
  size_t                   address_size,
  uint64_t                 amount,
  const char**             error_message);

/**
 * \brief Adds an output that sends the given value to an address.
 *
 * This function creates a transaction output carrying \p value (lovelace and native assets) for
 * \p address and appends it to the output list of the transaction body under construction.
 *
 * \param[in,out] state A pointer to the \ref cardano_builder_state_t tracking the transaction under
 *                      construction. This parameter must not be NULL.
 * \param[in] address A pointer to the \ref cardano_address_t receiving the value. The output holds a
 *                    reference to it.
 * \param[in] value A pointer to the \ref cardano_value_t to send. The output holds a reference to it.
 * \param[out] error_message A pointer that receives a static string describing the failure when the
 *                           function does not return \ref CARDANO_SUCCESS. It is left untouched on
 *                           success. This parameter must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the output was added, or an appropriate error code indicating the
 *         failure reason.
 */
cardano_error_t
cardano_builder_send_value(
  cardano_builder_state_t* state,
  cardano_address_t*       address,
  cardano_value_t*         value,
  const char**             error_message);

/**
 * \brief Adds an output that sends the given value to an address given as a string.
 *
 * This function parses the address string and delegates to \ref cardano_builder_send_value.
 *
 * \param[in,out] state A pointer to the \ref cardano_builder_state_t tracking the transaction under
 *                      construction. This parameter must not be NULL.
 * \param[in] address A pointer to the character string with the destination address.
 * \param[in] address_size The size of the address string in bytes.
 * \param[in] value A pointer to the \ref cardano_value_t to send. The output holds a reference to it.
 * \param[out] error_message A pointer that receives a static string describing the failure when the
 *                           function does not return \ref CARDANO_SUCCESS. It is left untouched on
 *                           success. This parameter must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the output was added, or an appropriate error code indicating the
 *         failure reason.
 */
cardano_error_t
cardano_builder_send_value_ex(
  cardano_builder_state_t* state,
  const char*              address,
  size_t                   address_size,
  cardano_value_t*         value,
  const char**             error_message);

/**
 * \brief Adds an output that locks the given amount of lovelace at a script address.
 *
 * This function creates a transaction output paying \p amount lovelace to \p script_address, attaches
 * the optional datum and appends it to the output list of the transaction body under construction.
 *
 * \param[in,out] state A pointer to the \ref cardano_builder_state_t tracking the transaction under
 *                      construction. This parameter must not be NULL.
 * \param[in] script_address A pointer to the \ref cardano_address_t of the script. The output holds a
 *                           reference to it.
 * \param[in] amount The amount of lovelace to lock.
 * \param[in] datum A pointer to the \ref cardano_datum_t attached to the output, or NULL when the
 *                  output carries no datum.
 * \param[out] error_message A pointer that receives a static string describing the failure when the
 *                           function does not return \ref CARDANO_SUCCESS. It is left untouched on
 *                           success. This parameter must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the output was added, or an appropriate error code indicating the
 *         failure reason.
 */
cardano_error_t
cardano_builder_lock_lovelace(
  cardano_builder_state_t* state,
  cardano_address_t*       script_address,
  uint64_t                 amount,
  cardano_datum_t*         datum,
  const char**             error_message);

/**
 * \brief Adds an output that locks the given amount of lovelace at a script address given as a string.
 *
 * This function parses the script address string and delegates to \ref cardano_builder_lock_lovelace.
 *
 * \param[in,out] state A pointer to the \ref cardano_builder_state_t tracking the transaction under
 *                      construction. This parameter must not be NULL.
 * \param[in] script_address A pointer to the character string with the script address.
 * \param[in] script_address_size The size of the script address string in bytes.
 * \param[in] amount The amount of lovelace to lock.
 * \param[in] datum A pointer to the \ref cardano_datum_t attached to the output, or NULL when the
 *                  output carries no datum.
 * \param[out] error_message A pointer that receives a static string describing the failure when the
 *                           function does not return \ref CARDANO_SUCCESS. It is left untouched on
 *                           success. This parameter must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the output was added, or an appropriate error code indicating the
 *         failure reason.
 */
cardano_error_t
cardano_builder_lock_lovelace_ex(
  cardano_builder_state_t* state,
  const char*              script_address,
  size_t                   script_address_size,
  uint64_t                 amount,
  cardano_datum_t*         datum,
  const char**             error_message);

/**
 * \brief Adds an output that locks the given value at a script address.
 *
 * This function creates a transaction output carrying \p value (lovelace and native assets) for
 * \p script_address, attaches the optional datum and appends it to the output list of the transaction
 * body under construction.
 *
 * \param[in,out] state A pointer to the \ref cardano_builder_state_t tracking the transaction under
 *                      construction. This parameter must not be NULL.
 * \param[in] script_address A pointer to the \ref cardano_address_t of the script. The output holds a
 *                           reference to it.
 * \param[in] value A pointer to the \ref cardano_value_t to lock. The output holds a reference to it.
 * \param[in] datum A pointer to the \ref cardano_datum_t attached to the output, or NULL when the
 *                  output carries no datum.
 * \param[out] error_message A pointer that receives a static string describing the failure when the
 *                           function does not return \ref CARDANO_SUCCESS. It is left untouched on
 *                           success. This parameter must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the output was added, or an appropriate error code indicating the
 *         failure reason.
 */
cardano_error_t
cardano_builder_lock_value(
  cardano_builder_state_t* state,
  cardano_address_t*       script_address,
  cardano_value_t*         value,
  cardano_datum_t*         datum,
  const char**             error_message);

/**
 * \brief Adds an output that locks the given value at a script address given as a string.
 *
 * This function parses the script address string and delegates to \ref cardano_builder_lock_value.
 *
 * \param[in,out] state A pointer to the \ref cardano_builder_state_t tracking the transaction under
 *                      construction. This parameter must not be NULL.
 * \param[in] script_address A pointer to the character string with the script address.
 * \param[in] script_address_size The size of the script address string in bytes.
 * \param[in] value A pointer to the \ref cardano_value_t to lock. The output holds a reference to it.
 * \param[in] datum A pointer to the \ref cardano_datum_t attached to the output, or NULL when the
 *                  output carries no datum.
 * \param[out] error_message A pointer that receives a static string describing the failure when the
 *                           function does not return \ref CARDANO_SUCCESS. It is left untouched on
 *                           success. This parameter must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the output was added, or an appropriate error code indicating the
 *         failure reason.
 */
cardano_error_t
cardano_builder_lock_value_ex(
  cardano_builder_state_t* state,
  const char*              script_address,
  size_t                   script_address_size,
  cardano_value_t*         value,
  cardano_datum_t*         datum,
  const char**             error_message);

/**
 * \brief Adds a fully formed output to the transaction.
 *
 * This function appends the given output to the output list of the transaction body under
 * construction.
 *
 * \param[in,out] state A pointer to the \ref cardano_builder_state_t tracking the transaction under
 *                      construction. This parameter must not be NULL.
 * \param[in] output A pointer to the \ref cardano_transaction_output_t to add. The list holds a
 *                   reference to it.
 *
 * \return \ref CARDANO_SUCCESS if the output was added, or an appropriate error code indicating the
 *         failure reason.
 */
cardano_error_t
cardano_builder_add_output(
  cardano_builder_state_t*      state,
  cardano_transaction_output_t* output);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_BUILDER_OUTPUTS_H
