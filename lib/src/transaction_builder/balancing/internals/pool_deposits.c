/**
 * \file pool_deposits.c
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

/* INCLUDES ******************************************************************/

#include "pool_deposits.h"

#include "unique_signers.h"

#include <cardano/certs/pool_registration_cert.h>
#include <cardano/transaction/sub_transaction.h>
#include <cardano/transaction_body/sub_transaction_body.h>

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Records a pool registered by a body and counts it if another body of the batch already registers it.
 *
 * \param[in,out] body_pools        The operator key hashes of the pools the body registers in the certificates
 *                                  inspected so far.
 * \param[in,out] batch_pools       The operator key hashes of the pools the batch registers in the bodies inspected
 *                                  so far.
 * \param[in]     operator_key_hash The operator key hash of the registered pool.
 * \param[in,out] count             The running count of pool deposits counted more than once.
 *
 * \return \ref CARDANO_SUCCESS if the pool was recorded, or an appropriate error code.
 */
static cardano_error_t
add_pool_registration(
  cardano_blake2b_hash_set_t* body_pools,
  cardano_blake2b_hash_set_t* batch_pools,
  cardano_blake2b_hash_t*     operator_key_hash,
  uint64_t*                   count)
{
  if (_cardano_blake2b_hash_set_has(body_pools, operator_key_hash))
  {
    return CARDANO_SUCCESS;
  }

  cardano_error_t result = cardano_blake2b_hash_set_add(body_pools, operator_key_hash);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  if (_cardano_blake2b_hash_set_has(batch_pools, operator_key_hash))
  {
    ++(*count);

    return CARDANO_SUCCESS;
  }

  return cardano_blake2b_hash_set_add(batch_pools, operator_key_hash);
}

/**
 * \brief Records the pools a body registers and counts the ones another body of the batch already registers.
 *
 * \param[in]     certificates The certificates of the body, or NULL when it has none.
 * \param[in,out] batch_pools  The operator key hashes of the pools the bodies inspected so far register. The pools of
 *                             this body are added to it.
 * \param[in,out] count        The running count of pool deposits counted more than once.
 *
 * \return \ref CARDANO_SUCCESS if the certificates were inspected, or an appropriate error code.
 */
static cardano_error_t
add_body_pool_registrations(
  cardano_certificate_set_t*  certificates,
  cardano_blake2b_hash_set_t* batch_pools,
  uint64_t*                   count)
{
  cardano_blake2b_hash_set_t* body_pools = NULL;

  cardano_error_t result = cardano_blake2b_hash_set_new(&body_pools);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  const size_t size = cardano_certificate_set_get_length(certificates);

  for (size_t i = 0U; (i < size) && (result == CARDANO_SUCCESS); ++i)
  {
    cardano_certificate_t* certificate = NULL;

    result = cardano_certificate_set_get(certificates, i, &certificate);
    cardano_certificate_unref(&certificate);

    cardano_cert_type_t type = CARDANO_CERT_TYPE_STAKE_REGISTRATION;

    if (result == CARDANO_SUCCESS)
    {
      result = cardano_cert_get_type(certificate, &type);
    }

    if ((result == CARDANO_SUCCESS) && (type == CARDANO_CERT_TYPE_POOL_REGISTRATION))
    {
      cardano_blake2b_hash_t* operator_key_hash = NULL;

      result = _cardano_get_pool_operator_key_hash(certificate, &operator_key_hash);

      if (result == CARDANO_SUCCESS)
      {
        result = add_pool_registration(body_pools, batch_pools, operator_key_hash, count);
      }

      cardano_blake2b_hash_unref(&operator_key_hash);
    }
  }

  cardano_blake2b_hash_set_unref(&body_pools);

  return result;
}

/* IMPLEMENTATION ************************************************************/

cardano_error_t
_cardano_get_pool_operator_key_hash(
  cardano_certificate_t*   certificate,
  cardano_blake2b_hash_t** operator_key_hash)
{
  cardano_pool_registration_cert_t* registration = NULL;

  cardano_error_t result = cardano_certificate_to_pool_registration(certificate, &registration);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  cardano_pool_params_t* params = NULL;

  result = cardano_pool_registration_cert_get_params(registration, &params);
  cardano_pool_registration_cert_unref(&registration);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = cardano_pool_params_get_operator_key_hash(params, operator_key_hash);
  cardano_pool_params_unref(&params);

  return result;
}

cardano_error_t
_cardano_count_repeated_pool_deposits(
  cardano_certificate_set_t*     certificates,
  cardano_sub_transaction_set_t* sub_transactions,
  uint64_t*                      count)
{
  if (count == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_blake2b_hash_set_t* batch_pools = NULL;

  cardano_error_t result = cardano_blake2b_hash_set_new(&batch_pools);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  uint64_t repeated = 0U;

  result = add_body_pool_registrations(certificates, batch_pools, &repeated);

  const size_t num_sub_transactions = cardano_sub_transaction_set_get_length(sub_transactions);

  for (size_t i = 0U; (i < num_sub_transactions) && (result == CARDANO_SUCCESS); ++i)
  {
    cardano_sub_transaction_t* sub_tx = NULL;

    result = cardano_sub_transaction_set_get(sub_transactions, i, &sub_tx);
    cardano_sub_transaction_unref(&sub_tx);

    if (result == CARDANO_SUCCESS)
    {
      cardano_sub_transaction_body_t* body = cardano_sub_transaction_get_body(sub_tx);
      cardano_sub_transaction_body_unref(&body);

      cardano_certificate_set_t* sub_certificates = cardano_sub_transaction_body_get_certificates(body);
      cardano_certificate_set_unref(&sub_certificates);

      result = add_body_pool_registrations(sub_certificates, batch_pools, &repeated);
    }
  }

  cardano_blake2b_hash_set_unref(&batch_pools);

  if (result == CARDANO_SUCCESS)
  {
    *count = repeated;
  }

  return result;
}
