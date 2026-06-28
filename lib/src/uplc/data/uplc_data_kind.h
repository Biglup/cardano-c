/**
 * \file uplc_data_kind.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_UPLC_DATA_UPLC_DATA_KIND_H
#define BIGLUP_LABS_INCLUDE_CARDANO_UPLC_DATA_UPLC_DATA_KIND_H

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief The kind of an arena Plutus-data node.
 *
 * Mirrors \ref cardano_plutus_data_kind_t but tags the lean, arena-allocated
 * data node the CEK machine walks directly. The integer values match the
 * library kind so a kind crosses the boundary by a plain cast.
 */
typedef enum
{
  /** \brief A constructor with an alternative tag and an ordered field list. */
  CARDANO_UPLC_DATA_KIND_CONSTR = 0,
  /** \brief A map of (key, value) data pairs. */
  CARDANO_UPLC_DATA_KIND_MAP = 1,
  /** \brief An ordered list of data items. */
  CARDANO_UPLC_DATA_KIND_LIST = 2,
  /** \brief An arbitrary-precision integer. */
  CARDANO_UPLC_DATA_KIND_INTEGER = 3,
  /** \brief A byte string. */
  CARDANO_UPLC_DATA_KIND_BYTES = 4
} cardano_uplc_data_kind_t;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BIGLUP_LABS_INCLUDE_CARDANO_UPLC_DATA_UPLC_DATA_KIND_H */
