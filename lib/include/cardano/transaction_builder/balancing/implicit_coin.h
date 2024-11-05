/**
 * \file implicit_coin.h
 *
 * \author angel.castillo
 * \date   Nov 02, 2024
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_IMPLICIT_COIN_H
#define BIGLUP_LABS_INCLUDE_CARDANO_IMPLICIT_COIN_H

/* INCLUDES ******************************************************************/

#include <cardano/error.h>
#include <cardano/protocol_params/protocol_parameters.h>
#include <cardano/transaction/transaction.h>
#include <cardano/typedefs.h>

/* STRUCTURES *****************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Represents the implicit coin values in a transaction.
 */
typedef struct cardano_implicit_coin_t
{
    /**
     * \brief The reward withdrawals in the transaction.
     */
    uint64_t withdrawals;

    /**
     * \brief Delegation registration deposits.
     */
    uint64_t deposits;

    /**
     * \brief Deposits returned.
     */
    uint64_t reclaim_deposits;
} cardano_implicit_coin_t;

/* DECLARATIONS **************************************************************/

/**
 * \brief Computes the implicit coin balance for a transaction.
 *
 * The `cardano_compute_implicit_coin` function calculates the implicit coin balance for a given transaction.
 *
 * \param[in] tx A pointer to the \ref cardano_transaction_t object representing the transaction.
 * \param[in] protocol_params A pointer to \ref cardano_protocol_parameters_t containing the protocol parameters necessary for accurate computation.
 * \param[out] implicit_coin A pointer to \ref cardano_implicit_coin_t where the computed implicit coin balance will be stored.
 *
 * \return \ref CARDANO_SUCCESS if the implicit coin balance was successfully computed, or an appropriate error code indicating failure.
 *
 * Usage Example:
 * \code{.c}
 * cardano_transaction_t* transaction = ...;             // Transaction object
 * cardano_protocol_parameters_t* protocol_params = ...; // Protocol parameters
 * cardano_implicit_coin_t implicit_coin = { 0 };
 *
 * cardano_error_t result = cardano_compute_implicit_coin(transaction, protocol_params, &implicit_coin);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // The implicit coin balance was successfully computed and stored in `implicit_coin`
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_compute_implicit_coin(
  cardano_transaction_t*         tx,
  cardano_protocol_parameters_t* protocol_params,
  cardano_implicit_coin_t*       implicit_coin);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_IMPLICIT_COIN_H