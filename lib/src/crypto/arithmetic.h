/**
 * \file arithmetic.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_CRYPTO_ARITHMETIC_H
#define BIGLUP_LABS_INCLUDE_CARDANO_CRYPTO_ARITHMETIC_H

/* INCLUDES ******************************************************************/

#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

/**
 * \brief Adds a 256-bit number to another 256-bit number multiplied by 8, both represented as byte arrays.
 *
 * This function computes the sum of two 256-bit numbers where the second number is first
 * multiplied by 8 (equivalent to left-shifting its bits by 3 positions). The operation is
 * carried out on a byte-by-byte basis for the first 28 bytes of the input arrays, with the
 * remaining 4 bytes of the array being added without multiplication. The result is stored
 * in a provided output buffer.
 *
 * \param[in] x A pointer to the first operand, a 256-bit number represented as a 32-byte array.
 * \param[in] y A pointer to the second operand, a 256-bit number also represented as a 32-byte
 *              array. The first 28 bytes of this array will be effectively multiplied by 8 before
 *              being added to the corresponding bytes of the first operand.
 * \param[out] out A pointer to the output buffer where the result of the operation will be stored.
 *                 This buffer must be at least 32 bytes in size to accommodate the result.
 *
 * \note It's important to ensure that all input arrays and the output buffer are properly
 *       allocated and that the output buffer can safely hold 32 bytes. The function implements
 *       carry-over logic for the byte-wise addition, with carries propagated through the
 *       calculation. If the sum of the final bytes plus any remaining carry exceeds the size
 *       of a byte, the overflow is ignored, effectively treating the operation as modulo
 *       2^256.
 */
void _cardano_crypto_add28_mul8(const byte_t* x, const byte_t* y, byte_t* out);

/**
 * \brief Adds two 256-bit numbers represented as byte arrays, including carry handling.
 *
 * This function performs the addition of two 256-bit numbers, each represented as a 32-byte
 * array. The addition includes carry handling across byte boundaries, making it suitable
 * for arithmetic operations on large numbers beyond the range of standard integer types.
 * The result of the addition is stored in the provided output buffer, `out`.
 *
 * \param[in] x A pointer to the first operand, a 256-bit number represented as a 32-byte array.
 * \param[in] y A pointer to the second operand, another 256-bit number represented similarly to
 *              the first operand.
 * \param[out] out A pointer to a 32-byte output buffer where the result of the addition will be stored.
 *
 * \note The function correctly handles carry-over during the addition, ensuring that any overflow
 *       from adding individual bytes (including the carry from the previous byte, if any) is
 *       accounted for in the next byte. If the final byte addition results in an overflow, this
 *       overflow is ignored, effectively treating the numbers as if they were modulo 2^256.
 */
void _cardano_crypto_add256bits(const byte_t* x, const byte_t* y, byte_t* out);

#endif // BIGLUP_LABS_INCLUDE_CARDANO_CRYPTO_ARITHMETIC_H