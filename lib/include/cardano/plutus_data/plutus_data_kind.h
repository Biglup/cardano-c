/**
 * \file plutus_data_kind.h
 *
 * \author angel.castillo
 * \date   Mar 20, 2024
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_PLUTUS_DATA_KIND_H
#define BIGLUP_LABS_INCLUDE_CARDANO_PLUTUS_DATA_KIND_H

/* INCLUDES ******************************************************************/

#include <cardano/error.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \enum cardano_plutus_data_kind_t
 * \brief The plutus data type kind.
 */
typedef enum
{
  /**
   * \brief Represents a specific constructor of a 'Sum Type' along with its arguments.
   */
  CARDANO_PLUTUS_DATA_KIND_CONSTR,

  /**
   * \brief A map of PlutusData as both key and values.
   */
  CARDANO_PLUTUS_DATA_KIND_MAP,

  /**
   * \brief A list of PlutusData.
   */
  CARDANO_PLUTUS_DATA_KIND_LIST,

  /**
   * \brief An integer.
   */
  CARDANO_PLUTUS_DATA_KIND_INTEGER,

  /**
   * \brief Bounded bytes.
   */
  CARDANO_PLUTUS_DATA_KIND_BYTES

} cardano_plutus_data_kind_t;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_PLUTUS_DATA_KIND_H
