/**
 * \file uplc_apply_params.h
 *
 * \author angel.castillo
 * \date   Jun 19, 2026
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_UPLC_UPLC_EVALUATOR_H
#define BIGLUP_LABS_INCLUDE_CARDANO_UPLC_UPLC_EVALUATOR_H

/* INCLUDES ******************************************************************/

#include <cardano/buffer.h>
#include <cardano/error.h>
#include <cardano/export.h>
#include <cardano/plutus_data/plutus_list.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Applies a list of Plutus-data parameters to a compiled Plutus script.
 *
 * Decodes the CBOR-wrapped, flat-encoded script in \p script_bytes into a
 * program, applies each parameter in \p params in order (\c params[0] first, so
 * it becomes the innermost argument), and re-encodes the result to the ledger
 * witness-set form: the new flat program wrapped in a single CBOR bytestring.
 *
 * The decode peels exactly one CBOR layer, and the output is single-CBOR-wrapped,
 * so a canonical input round-trips byte-for-byte. A non-canonical input
 * re-encodes to the canonical form, but the resulting program is structurally
 * equal to applying the parameters to the decoded input.
 *
 * The decoding and re-encoding run inside an internal arena that is freed before
 * this returns on every path. The result \p out_script is allocated outside that
 * arena and outlives it.
 *
 * \param[in] params The parameters to apply, a list of plutus-data values, or
 *            NULL to apply no parameters. An empty or NULL list re-encodes the
 *            script unchanged (up to canonicalization).
 * \param[in] script_bytes The compiled script bytes, CBOR-wrapped flat. May be
 *            NULL only when \p script_size is 0, which is rejected as a malformed
 *            script.
 * \param[in] script_size The number of bytes in \p script_bytes.
 * \param[out] out_script On success, set to a newly allocated buffer holding the
 *             CBOR-wrapped flat bytes of the parameterized script. The caller owns
 *             it and releases it with \ref cardano_buffer_unref. Left untouched on
 *             failure.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         \p out_script is NULL or \p script_bytes is NULL while \p script_size is
 *         non-zero, \ref CARDANO_ERROR_DECODING if the script bytes are not a
 *         valid CBOR-wrapped flat program, \ref CARDANO_ERROR_NOT_IMPLEMENTED if
 *         the decoded program carries a constant form with no flat serialization,
 *         or \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED if the arena or an output
 *         buffer cannot be allocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_uplc_apply_params_to_script(
  const cardano_plutus_list_t* params,
  const byte_t*                script_bytes,
  size_t                       script_size,
  cardano_buffer_t**           out_script);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BIGLUP_LABS_INCLUDE_CARDANO_UPLC_UPLC_EVALUATOR_H */
