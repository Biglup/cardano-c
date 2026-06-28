/**
 * \file uplc_flat_fuzzer.c
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

/* INCLUDES ******************************************************************/

#include <cardano/cardano.h>

#include "uplc/arena.h"
#include "uplc/flat_decode.h"
#include "uplc/flat_reader.h"

#include <cardano/uplc/uplc_machine.h>

/* DEFINITIONS ***************************************************************/

/**
 * \brief Drives the flat decoder and the CEK machine against arbitrary bytes.
 *
 * The flat decoder is consensus-critical: it parses untrusted on-chain script
 * bytes. This target feeds the raw fuzz input straight into the flat reader and
 * the program decoder. When a program decodes, it is evaluated under a bounded
 * budget so a single input cannot run forever. The arena is freed on every path,
 * so a leak in the decode/eval interior is observable to LeakSanitizer.
 */
int
LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
  cardano_uplc_arena_t* arena = NULL;

  if (cardano_uplc_arena_new(0U, &arena) != CARDANO_SUCCESS)
  {
    return 0;
  }

  cardano_uplc_flat_reader_t reader;

  if (cardano_uplc_flat_reader_init(&reader, data, size) != CARDANO_SUCCESS)
  {
    cardano_uplc_arena_free(&arena);
    return 0;
  }

  const cardano_uplc_program_t* program = NULL;

  cardano_error_t result = cardano_uplc_flat_decode_program(arena, &reader, &program);

  if ((result == CARDANO_SUCCESS) && (program != NULL))
  {
    const cardano_uplc_budget_t budget = { .cpu = 10000000, .mem = 10000000 };

    cardano_uplc_eval_result_t eval = { 0 };

    (void)cardano_uplc_evaluate(arena, program, CARDANO_UPLC_MACHINE_VERSION_V3, budget, &eval);
  }

  cardano_uplc_arena_free(&arena);

  return 0;
}
