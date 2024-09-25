/**
 * \file cbor_additional_info.h
 *
 * \author angel.castillo
 * \date   Sep 29, 2023
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_CBOR_ADDITIONAL_INFO_H
#define BIGLUP_LABS_INCLUDE_CARDANO_CBOR_ADDITIONAL_INFO_H

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Represents the 5-bit additional information included in a CBOR initial byte.
 *
 * The CBOR encoding format specifies an initial byte for each data item that includes both the major type
 * and additional information. The additional information field is a 5-bit value that can indicate a variety
 * of things depending on the context, such as the length of the data, special values like `false`, `true`,
 * `null`, or the need for more bytes to represent the data length.
 */
typedef enum
{
  /**
   * \brief Indicates the boolean value `false` (major type 7).
   */
  CARDANO_CBOR_ADDITIONAL_INFO_FALSE = 20,

  /**
   * \brief Indicates the boolean value `true` (major type 7).
   */
  CARDANO_CBOR_ADDITIONAL_INFO_TRUE = 21,

  /**
   * \brief Indicates the value `null` (major type 7).
   */
  CARDANO_CBOR_ADDITIONAL_INFO_NULL = 22,

  /**
   * \brief Indicates that the next byte contains the length of the data (major types 0-2, 4-5).
   */
  CARDANO_CBOR_ADDITIONAL_INFO_8BIT_DATA = 24,

  /**
   * \brief Indicates that the next 2 bytes contain the length of the data (major types 0-2, 4-5).
   */
  CARDANO_CBOR_ADDITIONAL_INFO_16BIT_DATA = 25,

  /**
   * \brief Indicates that the next 4 bytes contain the length of the data (major types 0-2, 4-5).
   */
  CARDANO_CBOR_ADDITIONAL_INFO_32BIT_DATA = 26,

  /**
   * \brief Indicates that the next 8 bytes contain the length of the data (major types 0-2, 4-5).
   */
  CARDANO_CBOR_ADDITIONAL_INFO_64BIT_DATA = 27,

  /**
   * \brief Indicates that the data item is of indefinite length (major types 2, 3, 4, 5).
   */
  CARDANO_CBOR_ADDITIONAL_INFO_INDEFINITE_LENGTH = 31
} cardano_cbor_additional_info_t;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_CBOR_ADDITIONAL_INFO_H