/**
 * \file cbor_initial_byte.c
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

/* INCLUDES ******************************************************************/

#include "./cbor_initial_byte.h"
#include <stdlib.h>

/* STRUCTS *******************************************************************/

typedef struct cardano_cbor_initial_byte_t
{
    byte_t initial_byte;
    size_t ref_count;
} cardano_cbor_initial_byte_t;

/* DEFINITIONS ****************************************************************/

cardano_cbor_initial_byte_t*
cardano_cbor_initial_byte_new(cbor_major_type_t major_type, cbor_additional_info_t additional_info)
{
  cardano_cbor_initial_byte_t* initial_byte = (cardano_cbor_initial_byte_t*)malloc(sizeof(cardano_cbor_initial_byte_t));

  if (initial_byte == NULL)
  {
    return NULL;
  }

  initial_byte->initial_byte = (byte_t)((byte_t)major_type << 5) | (byte_t)additional_info;
  initial_byte->ref_count    = 1;

  return initial_byte;
}

cardano_cbor_initial_byte_t*
cardano_cbor_initial_byte_from(byte_t initial_byte)
{
  cardano_cbor_initial_byte_t* initial_byte_obj = (cardano_cbor_initial_byte_t*)malloc(sizeof(cardano_cbor_initial_byte_t));

  if (initial_byte_obj == NULL)
  {
    return NULL;
  }

  initial_byte_obj->initial_byte = initial_byte;
  initial_byte_obj->ref_count    = 1;

  return initial_byte_obj;
}

cardano_error_t
cardano_cbor_initial_byte_get_packed(cardano_cbor_initial_byte_t* initial_byte, byte_t* packed)
{
  if ((initial_byte == NULL) || (packed == NULL))
  {
    return CARDANO_POINTER_IS_NULL;
  }

  *packed = initial_byte->initial_byte;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_cbor_initial_byte_get_major_type(cardano_cbor_initial_byte_t* initial_byte, cbor_major_type_t* major_type)
{
  if ((initial_byte == NULL) || (major_type == NULL))
  {
    return CARDANO_POINTER_IS_NULL;
  }

  *major_type = (cbor_major_type_t)(initial_byte->initial_byte >> 5);

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_cbor_initial_byte_get_additional_info(cardano_cbor_initial_byte_t* initial_byte, cbor_additional_info_t* additional_info)
{
  if ((initial_byte == NULL) || (additional_info == NULL))
  {
    return CARDANO_POINTER_IS_NULL;
  }

  *additional_info = (cbor_additional_info_t)initial_byte->initial_byte & (cbor_additional_info_t)0x1F;

  return CARDANO_SUCCESS;
}

void
cardano_cbor_initial_byte_unref(cardano_cbor_initial_byte_t** initial_byte)
{
  if (initial_byte == NULL)
  {
    return;
  }

  if (*initial_byte == NULL)
  {
    return;
  }

  cardano_cbor_initial_byte_t* reference = *initial_byte;

  if (reference->ref_count > 0U)
  {
    reference->ref_count -= 1U;
  }

  if (reference->ref_count == 0U)
  {
    free(*initial_byte);
    *initial_byte = NULL;
  }
}

void
cardano_cbor_initial_byte_ref(cardano_cbor_initial_byte_t* initial_byte)
{
  if (initial_byte == NULL)
  {
    return;
  }

  ++initial_byte->ref_count;
}

size_t
cardano_cbor_initial_byte_refcount(const cardano_cbor_initial_byte_t* initial_byte)
{
  if (initial_byte == NULL)
  {
    return 0;
  }

  return initial_byte->ref_count;
}

cardano_cbor_initial_byte_t*
cardano_cbor_initial_byte_move(cardano_cbor_initial_byte_t* initial_byte)
{
  if (initial_byte == NULL)
  {
    return NULL;
  }

  --initial_byte->ref_count;

  return initial_byte;
}
