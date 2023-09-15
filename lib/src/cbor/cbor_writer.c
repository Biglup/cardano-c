/**
 * \file cbor_writer.c
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

#include <pthread.h>
#include <stdlib.h>

/* DECLARATIONS **************************************************************/

typedef struct cardano_cbor_writer_t
{
    size_t ref_count;
} cardano_cbor_writer_t;

cardano_cbor_writer_t*
cardano_cbor_writer_new()
{
  cardano_cbor_writer_t* obj = (cardano_cbor_writer_t*)malloc(sizeof(cardano_cbor_writer_t));

  obj->ref_count = 1;

  return obj;
}

void
cardano_cbor_writer_unref(cardano_cbor_writer_t** cbor_writer)
{
  if (cbor_writer == NULL)
  {
    return;
  }

  if (*cbor_writer == NULL)
  {
    return;
  }

  cardano_cbor_writer_t* reference = *cbor_writer;

  if (reference->ref_count > 0)
  {
    reference->ref_count -= 1U;
  }

  if (reference->ref_count <= 0U)
  {
    free(reference);
    *cbor_writer = NULL;
  }
}

void
cardano_cbor_writer_ref(cardano_cbor_writer_t* cbor_writer)
{
  if (cbor_writer == NULL)
  {
    return;
  }

  cbor_writer->ref_count += 1U;
}

size_t
cardano_cbor_writer_refcount(const cardano_cbor_writer_t* cbor_writer)
{
  if (cbor_writer == NULL)
  {
    return 0;
  }

  return cbor_writer->ref_count;
}

cardano_cbor_writer_t*
cardano_cbor_writer_move(cardano_cbor_writer_t* cbor_writer)
{
  if (cbor_writer == NULL)
  {
    return NULL;
  }

  cbor_writer->ref_count -= 1U;

  return cbor_writer;
}