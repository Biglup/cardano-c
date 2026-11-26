/**
 * \file created_output_scripts.h
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

#ifndef CARDANO_CREATED_OUTPUT_SCRIPTS_H
#define CARDANO_CREATED_OUTPUT_SCRIPTS_H

#include <cardano/scripts/script.h>
#include <cardano/transaction/transaction.h>

/**
 * \brief Finds a CIP-172 reference script in outputs created by a batch, including collateral return.
 * \param[in] tx The top-level transaction containing the complete batch.
 * \param[in] hash The required script hash.
 * \param[out] script An owned reference, or NULL when the script is not present.
 * \return CARDANO_SUCCESS, CARDANO_ERROR_ELEMENT_NOT_FOUND, or an allocation/access error.
 * \note Call only for Dijkstra (protocol version 12 or later).
 */
cardano_error_t
cardano_builder_find_created_script(cardano_transaction_t* tx, const cardano_blake2b_hash_t* hash, cardano_script_t** script);

#endif /* CARDANO_CREATED_OUTPUT_SCRIPTS_H */
