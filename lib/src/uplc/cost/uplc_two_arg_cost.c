/**
 * \file uplc_two_arg_cost.c
 *
 * \author angel.castillo
 * \date   Jun 27, 2026
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

#include "uplc_two_arg_cost.h"

#include <stddef.h>
#include <stdint.h>

/* DEFINITIONS ***************************************************************/

int64_t
cardano_uplc_two_arg_cost_eval(const cardano_uplc_two_arg_cost_t* fn, int64_t x, int64_t y)
{
  if (fn == NULL)
  {
    return 0;
  }

  return cardano_uplc_two_arg_eval(fn, x, y);
}
