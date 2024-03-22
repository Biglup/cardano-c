/**
 * \file cbor_initial_byte.c
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

/* INCLUDES ******************************************************************/

#include "./cbor_initial_byte.h"

#include <cardano/object.h>

#include <stdlib.h>

#include "../config.h"

/* DEFINITIONS ****************************************************************/

byte_t
cardano_cbor_initial_byte_pack(const cardano_cbor_major_type_t major_type, const cardano_cbor_additional_info_t additional_info)
{
  return (byte_t)(((byte_t)major_type << 5) | (byte_t)additional_info);
}

cardano_cbor_major_type_t
cardano_cbor_initial_byte_get_major_type(const byte_t initial_byte)
{
  return (cardano_cbor_major_type_t)(initial_byte >> 5);
}

cardano_cbor_additional_info_t
cardano_cbor_initial_byte_get_additional_info(const byte_t initial_byte)
{
  return (cardano_cbor_additional_info_t)initial_byte & (cardano_cbor_additional_info_t)0x1F;
}
