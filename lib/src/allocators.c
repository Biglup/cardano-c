/**
 * \file allocators.c
 *
 * \author luisd.bianchi
 * \date   Mar 11, 2024
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

#include "allocators.h"

#include <stdlib.h>

/* DEFINITIONS ***************************************************************/

static _cardano_malloc_t  s_cardano_malloc  = malloc;
static _cardano_realloc_t s_cardano_realloc = realloc;
static _cardano_free_t    s_cardano_free    = free;

void*
_cardano_malloc(size_t size)
{
  return s_cardano_malloc(size);
}

void*
_cardano_realloc(void* ptr, size_t size)
{
  return s_cardano_realloc(ptr, size);
}

void
_cardano_free(void* ptr)
{
  s_cardano_free(ptr);
}

void
cardano_set_allocators(_cardano_malloc_t custom_malloc, _cardano_realloc_t custom_realloc, _cardano_free_t custom_free)
{
  s_cardano_malloc  = custom_malloc;
  s_cardano_realloc = custom_realloc;
  s_cardano_free    = custom_free;
}