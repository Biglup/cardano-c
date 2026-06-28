/**
 * \file bls.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_UPLC_BUILTINS_BLS_H
#define BIGLUP_LABS_INCLUDE_CARDANO_UPLC_BUILTINS_BLS_H

/* INCLUDES ******************************************************************/

#include <cardano/error.h>
#include <cardano/typedefs.h>
#include "../ast/uplc_term.h"

#include "../arena/uplc_arena.h"

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Length in bytes of a BLS12-381 G1 point in compressed serialization.
 */
#define CARDANO_UPLC_BLS_G1_COMPRESSED_SIZE ((size_t)48U)

/**
 * \brief Length in bytes of a BLS12-381 G2 point in compressed serialization.
 */
#define CARDANO_UPLC_BLS_G2_COMPRESSED_SIZE ((size_t)96U)

/**
 * \brief Uncompresses a G1 point and stores it as an arena BLS constant.
 *
 * The input must be exactly \ref CARDANO_UPLC_BLS_G1_COMPRESSED_SIZE bytes,
 * uncompress without error, and lie in the G1 subgroup. Any other length, a bad
 * encoding, or a point outside the subgroup yields \ref CARDANO_ERROR_DECODING.
 *
 * \param[in] arena The arena the constant is allocated from.
 * \param[in] bytes The compressed point bytes. May be NULL only when \p size is 0.
 * \param[in] size The number of bytes in \p bytes.
 * \param[out] constant On success, the G1 constant; left untouched on failure.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_DECODING for an
 *         invalid or out-of-subgroup point, or a propagated allocation error.
 */
cardano_error_t
cardano_uplc_int_bls_g1_from_compressed(
  cardano_uplc_arena_t*     arena,
  const byte_t*             bytes,
  size_t                    size,
  cardano_uplc_constant_t** constant);

/**
 * \brief Uncompresses a G2 point and stores it as an arena BLS constant.
 *
 * As \ref cardano_uplc_int_bls_g1_from_compressed but for G2: the input must be
 * exactly \ref CARDANO_UPLC_BLS_G2_COMPRESSED_SIZE bytes and lie in the G2
 * subgroup.
 *
 * \param[in] arena The arena the constant is allocated from.
 * \param[in] bytes The compressed point bytes. May be NULL only when \p size is 0.
 * \param[in] size The number of bytes in \p bytes.
 * \param[out] constant On success, the G2 constant; left untouched on failure.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_DECODING for an
 *         invalid or out-of-subgroup point, or a propagated allocation error.
 */
cardano_error_t
cardano_uplc_int_bls_g2_from_compressed(
  cardano_uplc_arena_t*     arena,
  const byte_t*             bytes,
  size_t                    size,
  cardano_uplc_constant_t** constant);

/**
 * \brief Compresses the G1 point a constant carries into a byte buffer.
 *
 * \param[in] constant A constant of kind \ref CARDANO_UPLC_TYPE_BLS_G1.
 * \param[out] out A buffer of at least \ref CARDANO_UPLC_BLS_G1_COMPRESSED_SIZE bytes.
 *
 * \return \ref CARDANO_SUCCESS on success or \ref CARDANO_ERROR_INVALID_ARGUMENT
 *         if \p constant is not a G1 constant.
 */
cardano_error_t
cardano_uplc_int_bls_g1_compress(const cardano_uplc_constant_t* constant, byte_t* out);

/**
 * \brief Compresses the G2 point a constant carries into a byte buffer.
 *
 * \param[in] constant A constant of kind \ref CARDANO_UPLC_TYPE_BLS_G2.
 * \param[out] out A buffer of at least \ref CARDANO_UPLC_BLS_G2_COMPRESSED_SIZE bytes.
 *
 * \return \ref CARDANO_SUCCESS on success or \ref CARDANO_ERROR_INVALID_ARGUMENT
 *         if \p constant is not a G2 constant.
 */
cardano_error_t
cardano_uplc_int_bls_g2_compress(const cardano_uplc_constant_t* constant, byte_t* out);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BIGLUP_LABS_INCLUDE_CARDANO_UPLC_BUILTINS_BLS_H */
