/**
 * \file uplc_flat_asan_driver.c
 *
 * \author angel.castillo
 * \date   Jun 21, 2026
 *
 * Copyright 2026 Biglup Labs
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

/*
 * Local, throwaway hardening driver. NOT a libFuzzer target (no _fuzzer suffix,
 * so the fuzz CMake glob ignores it). Builds with -fsanitize=address,undefined,
 * loads the .flat conformance seeds, and pounds the flat decoder + CEK machine
 * with the original bytes and a battery of deterministic mutations: truncations,
 * bit flips, byte-block corruptions and random strings. Any sanitizer report or
 * crash is a real consensus bug in lib/src/uplc.
 */

#include <cardano/cardano.h>

#include "uplc/arena.h"
#include "uplc/flat_decode.h"
#include "uplc/flat_reader.h"

#include <cardano/uplc/uplc_machine.h>

#include <dirent.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint64_t g_rng_state = 0x123456789abcdef0ULL;

static uint64_t
prv_next(void)
{
  g_rng_state ^= g_rng_state << 13;
  g_rng_state ^= g_rng_state >> 7;
  g_rng_state ^= g_rng_state << 17;
  return g_rng_state;
}

static void
prv_seed(uint64_t s)
{
  g_rng_state = s != 0U ? s : 0x9e3779b97f4a7c15ULL;
}

static void
prv_run_one(const uint8_t* data, size_t size)
{
  cardano_uplc_arena_t* arena = NULL;

  if (cardano_uplc_arena_new(0U, &arena) != CARDANO_SUCCESS)
  {
    return;
  }

  cardano_uplc_flat_reader_t reader;

  if (cardano_uplc_flat_reader_init(&reader, data, size) != CARDANO_SUCCESS)
  {
    cardano_uplc_arena_free(&arena);
    return;
  }

  const cardano_uplc_program_t* program = NULL;

  cardano_error_t result = cardano_uplc_flat_decode_program(arena, &reader, &program);

  if ((result == CARDANO_SUCCESS) && (program != NULL))
  {
    const cardano_uplc_budget_t budget = { .cpu = 10000000, .mem = 10000000 };
    cardano_uplc_eval_result_t  eval   = { 0 };
    (void)cardano_uplc_evaluate(arena, program, CARDANO_UPLC_MACHINE_VERSION_V3, budget, &eval);
  }

  cardano_uplc_arena_free(&arena);
}

static uint8_t*
prv_read_file(const char* path, size_t* out_size)
{
  FILE* f = fopen(path, "rb");

  if (f == NULL)
  {
    return NULL;
  }

  fseek(f, 0L, SEEK_END);
  long len = ftell(f);
  fseek(f, 0L, SEEK_SET);

  if (len < 0)
  {
    fclose(f);
    return NULL;
  }

  uint8_t* buf = (uint8_t*)malloc((size_t)len + 1U);

  if (buf == NULL)
  {
    fclose(f);
    return NULL;
  }

  size_t got = fread(buf, 1U, (size_t)len, f);
  fclose(f);
  *out_size = got;
  return buf;
}

static void
prv_mutate_and_run(const uint8_t* seed, size_t size)
{
  prv_run_one(seed, size);

  size_t step = (size > 512U) ? (size / 512U) : 1U;

  for (size_t prefix = 0U; prefix <= size; prefix += step)
  {
    prv_run_one(seed, prefix);
  }

  uint8_t* scratch = (uint8_t*)malloc(size + 1U);

  if (scratch == NULL)
  {
    return;
  }

  for (int rounds = 0; rounds < 200; ++rounds)
  {
    memcpy(scratch, seed, size);

    int kind = (int)(prv_next() % 4U);

    if (kind == 0)
    {
      size_t flips = (size_t)(prv_next() % 16U) + 1U;
      for (size_t i = 0U; (i < flips) && (size > 0U); ++i)
      {
        size_t  bit = (size_t)(prv_next() % (size * 8U));
        scratch[bit / 8U] ^= (uint8_t)(1U << (bit % 8U));
      }
    }
    else if (kind == 1)
    {
      size_t edits = (size_t)(prv_next() % 8U) + 1U;
      for (size_t i = 0U; (i < edits) && (size > 0U); ++i)
      {
        scratch[prv_next() % size] = (uint8_t)(prv_next() & 0xFFU);
      }
    }
    else if (kind == 2)
    {
      for (size_t i = 0U; i < size; ++i)
      {
        scratch[i] = (uint8_t)(prv_next() & 0xFFU);
      }
    }
    else
    {
      size_t trunc = (size == 0U) ? 0U : (size_t)(prv_next() % size);
      prv_run_one(scratch, trunc);
      continue;
    }

    prv_run_one(scratch, size);
  }

  free(scratch);
}

int
main(int argc, char** argv)
{
  const char* dir_path = (argc > 1) ? argv[1] : "/Users/angel/Sources/angel/plutus/plutus-benchmark/validation/data";

  prv_seed(0xC0FFEE1234ABCDEFULL);

  DIR* dir = opendir(dir_path);

  size_t seed_count = 0U;

  if (dir != NULL)
  {
    struct dirent* ent = NULL;

    while ((ent = readdir(dir)) != NULL)
    {
      const char* name = ent->d_name;
      size_t      nlen = strlen(name);

      if ((nlen < 5U) || (strcmp(name + nlen - 5U, ".flat") != 0))
      {
        continue;
      }

      char path[4096];
      snprintf(path, sizeof(path), "%s/%s", dir_path, name);

      size_t   size = 0U;
      uint8_t* data = prv_read_file(path, &size);

      if (data == NULL)
      {
        continue;
      }

      ++seed_count;

      for (int pass = 0; pass < 10; ++pass)
      {
        prv_mutate_and_run(data, size);
      }

      free(data);
    }

    closedir(dir);
  }

  for (int i = 0; i < 20000; ++i)
  {
    size_t  size = (size_t)(prv_next() % 256U);
    uint8_t buf[256];
    for (size_t j = 0U; j < size; ++j)
    {
      buf[j] = (uint8_t)(prv_next() & 0xFFU);
    }
    prv_run_one(buf, size);
  }

  printf("done: %zu seeds processed\n", seed_count);
  return 0;
}
