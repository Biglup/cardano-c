/**
 * \file native_script_type.h
 *
 * \author angel.castillo
 * \date   May 19, 2024
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
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY TYPE, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_NATIVE_SCRIPT_TYPE_H
#define BIGLUP_LABS_INCLUDE_CARDANO_NATIVE_SCRIPT_TYPE_H

/* INCLUDES ******************************************************************/

#include <cardano/export.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \enum cardano_native_script_type_t
 * \brief The native script type.
 *
 * This enumeration defines the types of native scripts that can be used in Cardano.
 */
typedef enum
{
  /**
   * \brief The script requires a specific signature.
   */
  CARDANO_NATIVE_SCRIPT_TYPE_REQUIRE_PUBKEY = 0,

  /**
   * \brief The script requires all sub-scripts to evaluate to true.
   */
  CARDANO_NATIVE_SCRIPT_TYPE_REQUIRE_ALL_OF = 1,

  /**
   * \brief The script requires any one of the sub-scripts to evaluate to true.
   */
  CARDANO_NATIVE_SCRIPT_TYPE_REQUIRE_ANY_OF = 2,

  /**
   * \brief The script requires at least N of the sub-scripts to evaluate to true.
   */
  CARDANO_NATIVE_SCRIPT_TYPE_REQUIRE_N_OF_K = 3,

  /**
   * \brief The script requires that the current slot is greater than or equal to a specified start time.
   */
  CARDANO_NATIVE_SCRIPT_TYPE_INVALID_BEFORE = 4,

  /**
   * \brief The script requires that the current slot is less than a specified expiry time.
   */
  CARDANO_NATIVE_SCRIPT_TYPE_INVALID_AFTER = 5
} cardano_native_script_type_t;

/**
 * \brief Converts native script types to their human readable form.
 *
 * \param[in] type The native script type to get the string representation for.
 * \return Human readable form of the given native script type.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_native_script_type_to_string(cardano_native_script_type_t type);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_NATIVE_SCRIPT_TYPE_H
