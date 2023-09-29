/**
 * \file cbor_tag.h
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

#ifndef CARDANO_CBOR_TAG_H
#define CARDANO_CBOR_TAG_H

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * Represents a CBOR semantic tag (major type 6).
 */
typedef enum
{
  /**
   * Tag value for RFC3339 date/time strings.
   */
  CBOR_TAG_DATE_TIME_STRING = 0,

  /**
   * Tag value for Epoch-based date/time strings.
   */
  CBOR_TAG_UNIX_TIME_SECONDS = 1,

  /**
   * Tag value for unsigned bignum encodings.
   */
  CBOR_TAG_UNSIGNED_BIG_NUM = 2,

  /**
   * Tag value for negative bignum encodings.
   */
  CBOR_TAG_NEGATIVE_BIG_NUM = 3,

  /**
   * Tag value for decimal fraction encodings.
   */
  CBOR_TAG_DECIMAL_FRACTION = 4,

  /**
   * Tag value for big float encodings.
   */
  CBOR_TAG_BIG_FLOAT = 5,

  /**
   * Tag value for byte strings, meant for later encoding to a base64url string representation.
   */
  CBOR_TAG_BASE64_URL_LATER_ENCODING = 21,

  /**
   * Tag value for byte strings, meant for later encoding to a base64 string representation.
   */
  CBOR_TAG_BASE64_STRING_LATER_ENCODING = 22,

  /**
   * Tag value for byte strings, meant for later encoding to a base16 string representation.
   */
  CBOR_TAG_BASE16_STRING_LATER_ENCODING = 23,

  /**
   * Tag value for byte strings containing embedded CBOR data item encodings.
   */
  CBOR_TAG_ENCODED_CBOR_DATA_ITEM = 24,

  /**
   * Tag value for Rational numbers, as defined in http://peteroupc.github.io/CBOR/rational.html.
   */
  CBOR_TAG_RATIONAL_NUMBER = 30,

  /**
   * Tag value for Uri strings, as defined in RFC3986.
   */
  CBOR_TAG_URI = 32,

  /**
   * Tag value for base64url-encoded text strings, as defined in RFC4648.
   */
  CBOR_TAG_BASE64_URL = 33,

  /**
   * Tag value for base64-encoded text strings, as defined in RFC4648.
   */
  CBOR_TAG_BASE64 = 34,

  /**
   * Tag value for regular expressions in Perl Compatible Regular Expressions / Javascript syntax.
   */
  CBOR_TAG_REGEX = 35,

  /**
   * Tag value for MIME messages (including all headers), as defined in RFC2045.
   */
  CBOR_TAG_MIME_MESSAGE = 36,

  /**
   * Tag value for the Self-Describe CBOR header (0xd9d9f7).
   */
  CBOR_TAG_SELF_DESCRIBE_CBOR = 55799
} cbor_tag_t;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // CARDANO_CBOR_TAG_H