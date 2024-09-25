/**
 * \file cbor_simple_value.h
 *
 * \author angel.castillo
 * \date   Sep 12, 2023
 *
 * Copyright 2023 Biglup Labs
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_CBOR_SIMPLE_VALUE_H
#define BIGLUP_LABS_INCLUDE_CARDANO_CBOR_SIMPLE_VALUE_H

/* INCLUDES ******************************************************************/

#include <cardano/export.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Represents a CBOR simple value (major type 7).
 *
 * These simple values are part of the CBOR data format as defined in RFC 7049, section 2.3,
 * representing commonly used simple data items. This enumeration includes the simple values
 * for 'false', 'true', 'null', and 'undefined', each of which has a specific role in the CBOR encoding
 * and interpretation process.
 */
typedef enum
{
  /**
   * \brief Represents the value 'false'.
   *
   * This value is used to represent the boolean false in CBOR-encoded data.
   */
  CARDANO_CBOR_SIMPLE_VALUE_FALSE = 20,

  /**
   * \brief Represents the value 'true'.
   *
   * This value is used to represent the boolean true in CBOR-encoded data.
   */
  CARDANO_CBOR_SIMPLE_VALUE_TRUE = 21,

  /**
   * \brief Represents the value 'null'.
   *
   * This value signifies a null reference or the absence of data in CBOR-encoded data.
   */
  CARDANO_CBOR_SIMPLE_VALUE_NULL = 22,

  /**
   * \brief Represents an undefined value.
   *
   * This value is used by an encoder as a substitute for a data item with an encoding problem,
   * indicating the absence of meaningful or correct data.
   */
  CARDANO_CBOR_SIMPLE_VALUE_UNDEFINED = 23
} cardano_cbor_simple_value_t;

/**
 * \brief Converts CBOR simple value to their human readable form.
 *
 * \param[in] simple_value The simple value to get the string representation for.
 * \return Human readable form of the given simple value. If the simple value is unknown,
 * returns "Simple Value: Unknown".
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_cbor_simple_value_to_string(cardano_cbor_simple_value_t simple_value);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_CBOR_SIMPLE_VALUE_H