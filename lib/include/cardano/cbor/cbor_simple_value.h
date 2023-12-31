/**
 * \file cbor_simple_value.h
 *
 * \author angel.castillo
 * \date   Sep 12, 2023
 *
 * \section LICENSE
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

#ifndef CARDANO_CBOR_SIMPLE_VALUE_H
#define CARDANO_CBOR_SIMPLE_VALUE_H

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * Represents a CBOR simple value (major type 7).
 */
typedef enum
{
  /**
   * Represents the value 'false'.
   */
  CBOR_SIMPLE_VALUE_FALSE = 20,

  /**
   * Represents the value 'true'.
   */
  CBOR_SIMPLE_VALUE_TRUE = 21,

  /**
   * Represents the value 'null'.
   */
  CBOR_SIMPLE_VALUE_NULL = 22,

  /**
   * Represents an undefined value, to be used by an encoder as a substitute for a data item with an encoding problem.
   */
  CBOR_SIMPLE_VALUE_UNDEFINED = 23
} cbor_simple_value_t;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // CARDANO_CBOR_SIMPLE_VALUE_H