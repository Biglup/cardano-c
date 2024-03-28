/**
 * \file arithmetic.c
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

/* INCLUDES ******************************************************************/

#include "arithmetic.h"

/* DEFINITIONS ****************************************************************/

void
_cardano_crypto_add28_mul8(const byte_t* x, const byte_t* y, byte_t* out)
{
  uint32_t carry = 0;

  for (int i = 0; i < 28; i++)
  {
    uint32_t r = x[i] + ((uint32_t)y[i] << 3) + carry;
    out[i]     = (uint8_t)(r & 0xFFu);
    carry      = r >> 8;
  }

  for (int i = 28; i < 32; i++)
  {
    uint32_t r = x[i] + carry;
    out[i]     = (uint8_t)(r & 0xFFu);
    carry      = r >> 8;
  }
}

void
_cardano_crypto_add256bits(const uint8_t* x, const uint8_t* y, uint8_t* out)
{
  uint32_t carry = 0;

  for (int i = 0; i < 32; i++)
  {
    uint32_t r = (uint32_t)x[i] + (uint32_t)y[i] + carry;
    out[i]     = (uint8_t)(r & 0xFFu);
    carry      = r >> 8;
  }
}
