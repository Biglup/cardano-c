/**
 * \file half.h
 *
 * \author angel.castillo
 * \date   Sep 09, 2023
 *
 * \section LICENSE
 *
 * Copyright 2023 Biglup Labs
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef CARDANO_CBOR_HALF_H
#define CARDANO_CBOR_HALF_H

/* INCLUDES ******************************************************************/

#include <cardano/error.h>
#include <cardano/typedefs.h>

/* PROTOTYPES ****************************************************************/

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

  /**
   * \brief Decodes a half-precision (16 bits) floating-point number from a byte array.
   *
   * Given a byte array of length 2 representing a 16-bit half-precision floating-point number,
   * this function decodes the number and returns its float equivalent. The input byte array
   * is assumed to be in big-endian format, i.e., the least significant byte is at index 1.
   *
   * The IEEE 754 standard for half-precision floating-point numbers is used for decoding.
   * The format consists of three components: a sign bit, a 5-bit exponent, and a 10-bit significand.
   *
   * \param[in] data A byte array containing the 16-bit half-precision floating-point number in big-endian format.
   * \param[in] size The size in bytes of the byte array.
   * \returns The decoded floating-point number in the standard float format (single-precision).
   *
   * https://www.rfc-editor.org/rfc/rfc7049#appendix-D
   */
  double cardano_decode_half(const byte_t* data, size_t size);

  /**
   * \brief Encodes a single-precision float into a half-precision (16 bits) floating-point number as a byte array.
   *
   * Given a single-precision float, this function encodes it into a 16-bit half-precision floating-point number
   * and returns it as a byte array of length 2 in big-endian format, i.e., the least significant byte is at index 1.
   *
   * The IEEE 754 standard for half-precision floating-point numbers is used for encoding.
   * The format consists of three components: a sign bit, a 5-bit exponent, and a 10-bit significand.
   *
   * \param[in] value A single-precision float to be encoded as a half-precision floating-point number.
   * \param[out] data The buffer where the encoded data will be written.
   * \param[out] max_size The size of the byte array buffer where the encoded data will be written.
   *
   * \returns A byte array containing the 16-bit half-precision floating-point number in big-endian format.
   */
  cardano_error_t cardano_encode_half(double value, byte_t* data, size_t max_size);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // CARDANO_CBOR_HALF_H