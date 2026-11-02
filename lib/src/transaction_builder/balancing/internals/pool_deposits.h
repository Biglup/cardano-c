/**
 * \file pool_deposits.h
 *
 * \author angel.castillo
 * \date   Nov 02, 2026
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_POOL_DEPOSITS_H
#define BIGLUP_LABS_INCLUDE_CARDANO_POOL_DEPOSITS_H

/* INCLUDES ******************************************************************/

#include <cardano/certs/certificate.h>
#include <cardano/certs/certificate_set.h>
#include <cardano/crypto/blake2b_hash.h>
#include <cardano/error.h>
#include <cardano/transaction_body/sub_transaction_set.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

/**
 * \brief Gets the key hash of the operator of the pool a pool registration certificate registers.
 *
 * The operator key hash identifies the pool: two registrations with the same operator key hash register the same pool.
 *
 * \param[in]  certificate       The pool registration certificate.
 * \param[out] operator_key_hash On success, the key hash of the pool operator. The caller must release it with
 *                               \ref cardano_blake2b_hash_unref.
 *
 * \return \ref CARDANO_SUCCESS if the operator key hash was retrieved, \ref CARDANO_ERROR_INVALID_CERTIFICATE_TYPE if the
 *         certificate is not a pool registration, or an appropriate error code.
 */
cardano_error_t
_cardano_get_pool_operator_key_hash(
  cardano_certificate_t*   certificate,
  cardano_blake2b_hash_t** operator_key_hash);

/**
 * \brief Counts the pool deposits that charging each body of a CIP-118 batch on its own counts more than once.
 *
 * Each body charges the pool deposit once per pool it registers, while the ledger charges it once per pool across the
 * whole batch. The count is the number of pools registered by each body, summed over the bodies, minus the number of
 * pools registered by the batch, so the pool deposits the batch pays are the per body deposits minus this count times
 * the pool deposit.
 *
 * \param[in]  certificates     The certificates of the top level body, or NULL when it has none.
 * \param[in]  sub_transactions The sub transactions of the batch, or NULL when there are none.
 * \param[out] count            On success, the number of pool deposits counted more than once.
 *
 * \return \ref CARDANO_SUCCESS if the certificates were inspected, or an appropriate error code.
 */
cardano_error_t
_cardano_count_repeated_pool_deposits(
  cardano_certificate_set_t*     certificates,
  cardano_sub_transaction_set_t* sub_transactions,
  uint64_t*                      count);

#endif // BIGLUP_LABS_INCLUDE_CARDANO_POOL_DEPOSITS_H
