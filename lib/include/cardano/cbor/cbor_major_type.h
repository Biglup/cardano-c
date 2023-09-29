/**
 * \file cbor_reader_state.h
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

#ifndef CARDANO_CBOR_MAJOR_TYPE_H
#define CARDANO_CBOR_MAJOR_TYPE_H

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * Represents CBOR Major Types, as defined in RFC7049 section 2.1.
 */
typedef enum
{
  /**
   * An unsigned integer in the range 0..264-1 inclusive. The value of the encoded item is the argument itself.
   */
  CBOR_MAJOR_TYPE_UNSIGNED_INTEGER = 0,

  /**
   * A negative integer in the range -264..-1 inclusive. The value of the item is -1 minus the argument.
   */
  CBOR_MAJOR_TYPE_NEGATIVE_INTEGER = 1,

  /**
   * A byte string. The number of bytes in the string is equal to the argument.
   */
  CBOR_MAJOR_TYPE_BYTE_STRING = 2,

  /**
   * A text string (Section 2) encoded as UTF-8 [RFC3629]. The number of bytes in the string is equal to the argument.
   */
  CBOR_MAJOR_TYPE_UTF8_STRING = 3,

  /**
   * An array of data items. In other formats, arrays are also called lists, sequences, or tuples (a "CBOR sequence"
   * is something slightly different, though [RFC8742]). The argument is the number of data items in the array.
   */
  CBOR_MAJOR_TYPE_ARRAY = 4,

  /**
   * A map of pairs of data items. Maps are also called tables, dictionaries, hashes, or objects (in JSON).
   */
  CBOR_MAJOR_TYPE_MAP = 5,

  /**
   * A tagged data item ("tag") whose tag number, an integer in the range 0..264-1 inclusive, is the argument and whose
   * enclosed data item (tag content) is the single encoded data item that follows the head.
   */
  CBOR_MAJOR_TYPE_TAG = 6,

  /**
   * Simple values, Floating-point numbers and the "break" stop code.
   */
  CBOR_MAJOR_TYPE_SIMPLE = 7
} cbor_major_type_t;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // CARDANO_CBOR_MAJOR_TYPE_H