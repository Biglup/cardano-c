/**
 * \file bls.c
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

/* INCLUDES ******************************************************************/

#include "bls.h"

#include <blst.h>

#include <stddef.h>

/* DEFINITIONS ***************************************************************/

cardano_error_t
cardano_uplc_int_bls_g1_from_compressed(
  cardano_uplc_arena_t*     arena,
  const byte_t*             bytes,
  size_t                    size,
  cardano_uplc_constant_t** constant)
{
  blst_p1_affine affine;
  blst_p1        point;

  if ((arena == NULL) || (constant == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if ((bytes == NULL) || (size != CARDANO_UPLC_BLS_G1_COMPRESSED_SIZE))
  {
    return CARDANO_ERROR_DECODING;
  }

  if (blst_p1_uncompress(&affine, bytes) != BLST_SUCCESS)
  {
    return CARDANO_ERROR_DECODING;
  }

  blst_p1_from_affine(&point, &affine);

  if (!blst_p1_in_g1(&point))
  {
    return CARDANO_ERROR_DECODING;
  }

  return cardano_uplc_constant_new_bls(arena, CARDANO_UPLC_TYPE_BLS_G1, &point, sizeof(point), constant);
}

cardano_error_t
cardano_uplc_int_bls_g2_from_compressed(
  cardano_uplc_arena_t*     arena,
  const byte_t*             bytes,
  size_t                    size,
  cardano_uplc_constant_t** constant)
{
  blst_p2_affine affine;
  blst_p2        point;

  if ((arena == NULL) || (constant == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if ((bytes == NULL) || (size != CARDANO_UPLC_BLS_G2_COMPRESSED_SIZE))
  {
    return CARDANO_ERROR_DECODING;
  }

  if (blst_p2_uncompress(&affine, bytes) != BLST_SUCCESS)
  {
    return CARDANO_ERROR_DECODING;
  }

  blst_p2_from_affine(&point, &affine);

  if (!blst_p2_in_g2(&point))
  {
    return CARDANO_ERROR_DECODING;
  }

  return cardano_uplc_constant_new_bls(arena, CARDANO_UPLC_TYPE_BLS_G2, &point, sizeof(point), constant);
}

cardano_error_t
cardano_uplc_int_bls_g1_compress(const cardano_uplc_constant_t* constant, byte_t* out)
{
  if ((constant == NULL) || (out == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (constant->kind != CARDANO_UPLC_TYPE_BLS_G1)
  {
    return CARDANO_ERROR_INVALID_ARGUMENT;
  }

  blst_p1_compress(out, (const blst_p1*)constant->as.bls);

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_uplc_int_bls_g2_compress(const cardano_uplc_constant_t* constant, byte_t* out)
{
  if ((constant == NULL) || (out == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (constant->kind != CARDANO_UPLC_TYPE_BLS_G2)
  {
    return CARDANO_ERROR_INVALID_ARGUMENT;
  }

  blst_p2_compress(out, (const blst_p2*)constant->as.bls);

  return CARDANO_SUCCESS;
}
