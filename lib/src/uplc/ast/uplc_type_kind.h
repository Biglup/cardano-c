/**
 * \file uplc_type_kind.h
 *
 * \author angel.castillo
 * \date   Jun 18, 2026
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_UPLC_AST_UPLC_TYPE_KIND_H
#define BIGLUP_LABS_INCLUDE_CARDANO_UPLC_AST_UPLC_TYPE_KIND_H

/* INCLUDES ******************************************************************/

#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief The kind of a constant in the constant universe.
 *
 * Drives the active arm of \ref cardano_uplc_constant_t and the shape of a
 * \ref cardano_uplc_type_t.
 */
typedef enum
{
  /** \brief Arbitrary-precision integer. */
  CARDANO_UPLC_TYPE_INTEGER = 0,
  /** \brief Byte string. */
  CARDANO_UPLC_TYPE_BYTE_STRING = 1,
  /** \brief UTF-8 text string. */
  CARDANO_UPLC_TYPE_STRING = 2,
  /** \brief The unit type. */
  CARDANO_UPLC_TYPE_UNIT = 3,
  /** \brief Boolean. */
  CARDANO_UPLC_TYPE_BOOL = 4,
  /** \brief Homogeneous list carrying its element type. */
  CARDANO_UPLC_TYPE_LIST = 5,
  /** \brief Pair carrying its two component types. */
  CARDANO_UPLC_TYPE_PAIR = 6,
  /** \brief Plutus data. */
  CARDANO_UPLC_TYPE_DATA = 7,
  /** \brief BLS12-381 G1 group element. */
  CARDANO_UPLC_TYPE_BLS_G1 = 8,
  /** \brief BLS12-381 G2 group element. */
  CARDANO_UPLC_TYPE_BLS_G2 = 9,
  /** \brief BLS12-381 Miller-loop result. */
  CARDANO_UPLC_TYPE_BLS_ML_RESULT = 10,
  /** \brief Homogeneous array carrying its element type (Plutus V4). */
  CARDANO_UPLC_TYPE_ARRAY = 11,
  /** \brief Multi-asset value: a sorted map of policy to sorted map of token to amount (Plutus V4). */
  CARDANO_UPLC_TYPE_VALUE = 12
} cardano_uplc_type_kind_t;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BIGLUP_LABS_INCLUDE_CARDANO_UPLC_AST_UPLC_TYPE_KIND_H */
