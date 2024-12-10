/**
 * \file utf8.h
 *
 * \author angel.castillo
 * \date   Nov 06, 2024
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_UTF8_H
#define BIGLUP_LABS_INCLUDE_CARDANO_UTF8_H

/* INCLUDES ******************************************************************/

#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

/**
 * \brief Determines the length of a UTF-8 encoded character based on its first byte.
 *
 * This function analyzes the first byte of a UTF-8 sequence and returns the
 * total number of bytes required to encode the character. It supports 1-byte
 * (ASCII), 2-byte, 3-byte, and 4-byte UTF-8 sequences.
 *
 * \param[in] first_byte The first byte of a UTF-8 encoded character.
 *
 * \return The length of the UTF-8 sequence:
 * - `1` for a 1-byte sequence (ASCII characters).
 * - `2` for a 2-byte sequence.
 * - `3` for a 3-byte sequence.
 * - `4` for a 4-byte sequence.
 * - `0` if the byte does not match a valid UTF-8 sequence start byte.
 */
size_t
cardano_utf8_sequence_length(byte_t first_byte);

/**
 * \brief Parses a single hexadecimal character and returns its integer value.
 *
 * \param[in] c The character to parse.
 *
 * \return
 * - The integer value of the hexadecimal character (0-15) if valid.
 * - -1 if the character is not a valid hexadecimal digit.
 */
int32_t
cardano_parse_hex_digit(char c);

/**
 * \brief Parses a Unicode escape sequence and returns its corresponding Unicode code point.
 *
 * This function interprets a Unicode escape sequence in the form of `\uXXXX`, where `XXXX`
 * represents four hexadecimal digits, and converts it into a Unicode code point.
 *
 * \param[in] str A pointer to a 4-character string containing the hexadecimal digits of the Unicode escape sequence (e.g., "0041" for '\\u0041').
 *
 * \return The integer value of the Unicode code point represented by the escape sequence.
 *         Returns -1 if the input contains invalid hexadecimal characters.
 *
 * \note This function assumes the input string contains exactly 4 characters representing a valid Unicode escape sequence.
 *       It does not validate the resulting code point against Unicode ranges.
 */
int32_t
cardano_parse_unicode_escape(const char* str);

/**
 * \brief Encodes a Unicode code point into its UTF-8 representation.
 *
 * This function converts a Unicode code point into a UTF-8 encoded sequence and writes it to the provided buffer.
 * The function supports encoding for code points in the valid Unicode range (U+0000 to U+10FFFF).
 *
 * \param[in] codepoint The Unicode code point to encode.
 * \param[out] out A pointer to the character buffer where the UTF-8 encoded bytes will be stored.
 *                 The buffer must be large enough to accommodate up to 4 bytes.
 *
 * \return The number of bytes written to the buffer.
 *         Returns 0 if the codepoint is invalid or outside the range of valid Unicode code points.
 */
size_t
cardano_encode_utf8(int32_t codepoint, char* out);

/**
 * \brief Decodes a Unicode escape sequence (e.g., `\uXXXX` or a surrogate pair `\uXXXX\uXXXX`) into UTF-8.
 *
 * \param[in] str A pointer to the input string containing the Unicode escape sequence.
 *                The sequence should start with `\u` and be at least 6 characters long.
 * \param[in] len The length of the input string. It must be at least 6 for a single Unicode escape and at least 12 for a surrogate pair.
 * \param[out] out A buffer to store the resulting UTF-8-encoded character. The buffer must have enough space to hold up to 4 bytes.
 *
 * \return The number of bytes written to the `out` buffer. Returns 0 if decoding fails due to invalid input or insufficient length.
 */
size_t
cardano_decode_unicode_sequence(const char* str, size_t len, char* out);

#endif // BIGLUP_LABS_INCLUDE_CARDANO_UTF8_H