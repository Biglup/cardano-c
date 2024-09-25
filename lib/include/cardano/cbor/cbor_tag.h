/**
 * \file cbor_tag.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_CBOR_TAG_H
#define BIGLUP_LABS_INCLUDE_CARDANO_CBOR_TAG_H

/* INCLUDES ******************************************************************/

#include <cardano/export.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Represents a CBOR semantic tag (major type 6).
 *
 * Semantic tags in CBOR (Concise Binary Object Representation) provide additional context
 * to the data items that follow them, as defined in the CBOR standard (RFC 7049). These tags
 * indicate how the subsequent data should be interpreted, ranging from date/time formats
 * to various encoding schemes and specialized data types.
 */
typedef enum
{
  /**
   * \brief Tag value for RFC3339 date/time strings.
   *
   * Indicates that the following string data item is formatted according
   * to the RFC3339 specification for date and time.
   */
  CARDANO_CBOR_TAG_DATE_TIME_STRING = 0,

  /**
   * \brief Tag value for Epoch-based date/time in seconds.
   *
   * Denotes that the following integer data item represents a date and time as
   * the number of seconds elapsed since the Unix epoch (1970-01-01T00:00Z).
   */
  CARDANO_CBOR_TAG_UNIX_TIME_SECONDS = 1,

  /**
   * \brief Tag value for unsigned bignum encodings.
   *
   * Used to encode arbitrarily large unsigned integers that cannot fit within
   * the standard integer data item types.
   */
  CARDANO_CBOR_TAG_UNSIGNED_BIG_NUM = 2,

  /**
   * \brief Tag value for negative bignum encodings.
   *
   * Represents arbitrarily large negative integers, complementing the unsigned
   * bignum encoding for handling integers beyond the built-in integer types.
   */
  CARDANO_CBOR_TAG_NEGATIVE_BIG_NUM = 3,

  /**
   * \brief Tag value for decimal fraction encodings.
   *
   * Allows for the precise representation of decimal numbers using a base-10
   * exponent notation. Followed by an array of two integers: the exponent and
   * the significand.
   */
  CARDANO_CBOR_TAG_DECIMAL_FRACTION = 4,

  /**
   * \brief Tag value for big float encodings.
   *
   * Encodes floating-point numbers with arbitrary precision. Followed by an
   * array of two integers representing the base-2 exponent and significand.
   */
  CARDANO_CBOR_TAG_BIG_FLOAT = 5,

  /**
   * \brief Tag value for byte strings containing embedded CBOR data item encodings.
   */
  CARDANO_ENCODED_CBOR_DATA_ITEM = 24,

  /**
   * \brief Tag value for Rational numbers, as defined in http://peteroupc.github.io/CBOR/rational.html.
   */
  CARDANO_ENCODED_CBOR_RATIONAL_NUMBER = 30,

  /**
   * \brief Tag value for `set<a> = #6.258([* a]) / [* a]`, `nonempty_set<a> = #6.258([+ a]) / [+ a]`, `nonempty_oset<a> = #6.258([+ a]) / [+ a]`
   */
  CARDANO_CBOR_TAG_SET = 258,

  /**
   * \brief Tag value for the Self-Describe CBOR header (0xd9d9f7).
   *
   * When placed at the beginning of a CBOR document, this tag signals that the
   * document is encoded in CBOR, facilitating content type detection.
   */
  CARDANO_CBOR_TAG_SELF_DESCRIBE_CBOR = 55799
} cardano_cbor_tag_t;

/**
 * \brief Converts CBOR tags to their human readable form if possible.
 *
 * \param[in] tag The tag to get the string representation for.
 * \return Human readable form of the given tag. If the tag is unknown, returns "Tag: Custom".
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_cbor_tag_to_string(cardano_cbor_tag_t tag);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_CBOR_TAG_H