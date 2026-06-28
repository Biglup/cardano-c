/**
 * \file flat_encode.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_UPLC_FLAT_FLAT_ENCODE_H
#define BIGLUP_LABS_INCLUDE_CARDANO_UPLC_FLAT_FLAT_ENCODE_H

/* INCLUDES ******************************************************************/

#include "../ast/uplc_program.h"
#include <cardano/buffer.h>
#include <cardano/error.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Flat-encodes a program into a newly allocated byte buffer.
 *
 * Internal flat encoder behind \ref cardano_uplc_flat_encode_program. Writes the
 * three version words, then the term, then a byte-aligning filler, producing the
 * canonical flat byte stream and the exact inverse of
 * \ref cardano_uplc_flat_decode_program. The \c array, \c value and
 * \c bls12_381_mlresult constant kinds have no flat serialization and are rejected.
 * The caller is expected to have validated its arguments; the \c cardano_uplc_int_
 * prefix marks this entry module-internal.
 *
 * \param[in] program The program to encode. Must not be NULL and must carry a
 *            non-NULL term.
 * \param[out] out_flat On success, the flat bytes; the caller releases them with
 *             \ref cardano_buffer_unref. Left untouched on failure.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         \p program, its term, or \p out_flat is NULL,
 *         \ref CARDANO_ERROR_INVALID_ARGUMENT for an unknown term kind or a
 *         constant kind with no flat serialization, or
 *         \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED if a buffer cannot be grown.
 */
cardano_error_t
cardano_uplc_int_flat_encode_program(
  const cardano_uplc_program_t* program,
  cardano_buffer_t**            out_flat);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BIGLUP_LABS_INCLUDE_CARDANO_UPLC_FLAT_FLAT_ENCODE_H */
