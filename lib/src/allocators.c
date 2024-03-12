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

#include <cardano/allocators.h>

#include <stdlib.h>

/* DEFINITIONS ***************************************************************/

_cardano_malloc_t  _cardano_malloc  = malloc;
_cardano_realloc_t _cardano_realloc = realloc;
_cardano_free_t    _cardano_free    = free;

void
cardano_set_allocators(_cardano_malloc_t custom_malloc, _cardano_realloc_t custom_realloc, _cardano_free_t custom_free)
{
  _cardano_malloc  = custom_malloc;
  _cardano_realloc = custom_realloc;
  _cardano_free    = custom_free;
}