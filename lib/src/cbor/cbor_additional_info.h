/**
 * \file cbor_additional_info.h
 *
 * \author angel.castillo
 * \date   Sep 29, 2023
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

#ifndef CARDANO_CBOR_ADDITIONAL_INFO_H
#define CARDANO_CBOR_ADDITIONAL_INFO_H

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * Represents the 5-bit additional information included in a CBOR initial byte.
 */
typedef enum
{
  CBOR_ADDITIONAL_INFO_FALSE             = 20,
  CBOR_ADDITIONAL_INFO_TRUE              = 21,
  CBOR_ADDITIONAL_INFO_NULL              = 22,
  CBOR_ADDITIONAL_INFO_8BIT_DATA         = 24,
  CBOR_ADDITIONAL_INFO_16BIT_DATA        = 25,
  CBOR_ADDITIONAL_INFO_32BIT_DATA        = 26,
  CBOR_ADDITIONAL_INFO_64BIT_DATA        = 27,
  CBOR_ADDITIONAL_INFO_INDEFINITE_LENGTH = 31
} cbor_additional_info_t;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // CARDANO_CBOR_ADDITIONAL_INFO_H