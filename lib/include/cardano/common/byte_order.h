/**
 * \file byte_order.h
 *
 * \author luisd.bianchi
 * \date   Jun 07, 2024
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_BYTE_ORDER_H
#define BIGLUP_LABS_INCLUDE_CARDANO_BYTE_ORDER_H

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Enumerates the possible byte order types for endianness interpretation.
 *
 * This enumeration is used to specify the byte order of data being processed,
 * particularly when bytes need to be interpreted as numeric values. It supports
 * specifying little-endian and big-endian byte orders.
 */
typedef enum
{
  /**
   * \brief Little-endian byte order. In this byte_order, the least significant byte (LSB)
   * is placed at the smallest address, and the most significant byte (MSB) is placed
   * at the largest address. This is the standard byte order used in x86 processors.
   */
  CARDANO_BYTE_ORDER_LITTLE_ENDIAN = 0,

  /**
   * \brief Big-endian byte order. In this byte_order, the most significant byte (MSB) is
   * placed at the smallest address, and the least significant byte (LSB) is placed
   * at the largest address. This is commonly used in network protocols (network byte order).
   */
  CARDANO_BYTE_ORDER_BIG_ENDIAN = 1
} cardano_byte_order_t;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_BYTE_ORDER_H