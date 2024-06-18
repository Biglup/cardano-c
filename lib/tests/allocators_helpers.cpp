/**
 * \file allocators_helpers.cpp
 *
 * \author angel.castillo
 * \date   Mar 13, 2024
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

#include "./allocators_helpers.h"

#include <stdlib.h>

/* STATIC VARIABLES **********************************************************/

static int malloc_run_count  = 0;
static int realloc_run_count = 0;
static int free_run_count    = 0;

/* DEFINITIONS ***************************************************************/

void
reset_allocators_run_count()
{
  malloc_run_count  = 0;
  realloc_run_count = 0;
  free_run_count    = 0;
}

void*
fail_right_away_malloc(const size_t size)
{
  return NULL;
}

void*
fail_after_one_malloc(const size_t size)
{
  if (malloc_run_count == 0)
  {
    malloc_run_count++;
    return malloc(size);
  }
  return NULL;
}

void*
fail_after_two_malloc(const size_t size)
{
  if (malloc_run_count < 2)
  {
    malloc_run_count++;
    return malloc(size);
  }
  return NULL;
}

void*
fail_after_three_malloc(const size_t size)
{
  if (malloc_run_count < 3)
  {
    malloc_run_count++;
    return malloc(size);
  }
  return NULL;
}

void*
fail_after_four_malloc(size_t size)
{
  if (malloc_run_count < 4)
  {
    malloc_run_count++;
    return malloc(size);
  }
  return NULL;
}

void*
fail_after_five_malloc(size_t size)
{
  if (malloc_run_count < 5)
  {
    malloc_run_count++;
    return malloc(size);
  }
  return NULL;
}

void*
fail_after_six_malloc(size_t size)
{
  if (malloc_run_count < 6)
  {
    malloc_run_count++;
    return malloc(size);
  }
  return NULL;
}

void*
fail_after_seventh_malloc(size_t size)
{
  if (malloc_run_count < 7)
  {
    malloc_run_count++;
    return malloc(size);
  }
  return NULL;
}

void*
fail_after_eight_malloc(size_t size)
{
  if (malloc_run_count < 8)
  {
    malloc_run_count++;
    return malloc(size);
  }
  return NULL;
}

void*
fail_after_nine_malloc(size_t size)
{
  if (malloc_run_count < 9)
  {
    malloc_run_count++;
    return malloc(size);
  }
  return NULL;
}

void*
fail_after_thirteen_malloc(size_t size)
{
  if (malloc_run_count < 13)
  {
    malloc_run_count++;
    return malloc(size);
  }
  return NULL;
}

void*
fail_after_fourteen_malloc(size_t size)
{
  if (malloc_run_count < 14)
  {
    malloc_run_count++;
    return malloc(size);
  }
  return NULL;
}

void*
fail_after_thirty_seven_malloc(size_t size)
{
  if (malloc_run_count < 37)
  {
    malloc_run_count++;
    return malloc(size);
  }
  return NULL;
}

void*
fail_right_away_realloc(void* const ptr, const size_t size)
{
  return NULL;
}
