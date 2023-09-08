/**
* \file half.c
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

/* INCLUDES ******************************************************************/

#include <cardano/endian.h>
#include <cardano/error.h>
#include <cardano/typedefs.h>

#include <cardano/cbor/half.h>

#include <math.h>
#include <stdlib.h>

/* DEFINITIONS ***************************************************************/

double
cardano_decode_half(const byte_t* data, const size_t size)
{
  const uint16_t half = (data[0] << 8) + data[1];
  const uint32_t exp  = (half >> 10) & 0x1f;
  const uint32_t mant = half & 0x03ff;

  double val = 0;

  if (exp == 0)
  {
    val = ldexp(mant, -24);
  }
  else if (exp != 31)
  {
    val = ldexp(mant + 1024, exp - 25);
  }
  else
  {
    val = mant == 0 ? strtod("Inf", NULL) : strtod("NaN", NULL);
  }

  return half & 0x8000 ? -val : val;
}

cardano_error_t
cardano_encode_half(double value, byte_t* data, size_t max_size)
{
  const uint32_t u = 0;
  cardano_write_float32_be(value, (byte_t*)&u, 4, 0);

  if ((u & 0x1fff) != 0)
  {
    return CARDANO_ERROR_LOSS_OF_PRECISION;
  }

  int16_t        s16  = (u >> 16) & 0x8000;
  const uint32_t exp  = (u >> 23) & 0xff;
  const uint32_t mant = u & 0x7fffff;

  if (exp >= 113 && exp <= 142)
  {
    s16 += ((exp - 112) << 10) + (mant >> 13);
  }
  else if (exp >= 103 && exp < 113)
  {
    if (mant & ((1 << (126 - exp)) - 1))
    {
      return CARDANO_ERROR_LOSS_OF_PRECISION;
    }
    s16 += (mant + 0x800000) >> (126 - exp);
  }
  else
  {
    return CARDANO_ERROR_LOSS_OF_PRECISION;
  }

  data[0] = (s16 >> 8) & 0xff;
  data[1] = s16 & 0xff;

  return CARDANO_SUCCESS;
}
