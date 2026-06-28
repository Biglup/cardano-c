/**
 * \file uplc_builtin_semantics.c
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

#include "uplc_builtin_semantics.h"
#include "../ast/uplc_lang_version.h"

#include <stdint.h>

/* CONSTANTS *****************************************************************/

/**
 * \brief The protocol major version at which the Chang upgrade activated.
 *
 * At and above this version V1/V2 select semantics B.
 */
static const uint64_t CARDANO_UPLC_CHANG_PROTOCOL_VERSION = 9U;

/**
 * \brief The protocol major version at which the Van-Rossem upgrade activated.
 *
 * At and above this version V1/V2 select semantics D and V3 selects E.
 */
static const uint64_t CARDANO_UPLC_VAN_ROSSEM_PROTOCOL_VERSION = 11U;

/* DEFINITIONS ***************************************************************/

cardano_uplc_builtin_semantics_t
cardano_uplc_builtin_semantics_for_language(cardano_uplc_lang_version_t lang_version)
{
  cardano_uplc_builtin_semantics_t result = CARDANO_UPLC_SEMANTICS_C;

  switch (lang_version)
  {
    case CARDANO_UPLC_LANG_VERSION_V1:
    case CARDANO_UPLC_LANG_VERSION_V2:
    {
      result = CARDANO_UPLC_SEMANTICS_B;
      break;
    }
    case CARDANO_UPLC_LANG_VERSION_V3:
    case CARDANO_UPLC_LANG_VERSION_V4:
    {
      result = CARDANO_UPLC_SEMANTICS_C;
      break;
    }
    default:
    {
      break;
    }
  }

  return result;
}

cardano_uplc_builtin_semantics_t
cardano_uplc_builtin_semantics_for_language_and_protocol(
  cardano_uplc_lang_version_t lang_version,
  uint64_t                    protocol_major)
{
  cardano_uplc_builtin_semantics_t result = CARDANO_UPLC_SEMANTICS_C;

  switch (lang_version)
  {
    case CARDANO_UPLC_LANG_VERSION_V1:
    case CARDANO_UPLC_LANG_VERSION_V2:
    {
      if (protocol_major >= CARDANO_UPLC_VAN_ROSSEM_PROTOCOL_VERSION)
      {
        result = CARDANO_UPLC_SEMANTICS_D;
      }
      else if (protocol_major >= CARDANO_UPLC_CHANG_PROTOCOL_VERSION)
      {
        result = CARDANO_UPLC_SEMANTICS_B;
      }
      else
      {
        result = CARDANO_UPLC_SEMANTICS_A;
      }
      break;
    }
    case CARDANO_UPLC_LANG_VERSION_V3:
    case CARDANO_UPLC_LANG_VERSION_V4:
    {
      result = (protocol_major >= CARDANO_UPLC_VAN_ROSSEM_PROTOCOL_VERSION) ? CARDANO_UPLC_SEMANTICS_E
                                                                            : CARDANO_UPLC_SEMANTICS_C;
      break;
    }
    default:
    {
      break;
    }
  }

  return result;
}

bool
cardano_uplc_semantics_costs_strings_by_utf8_bytes(cardano_uplc_builtin_semantics_t semantics)
{
  bool result = false;

  switch (semantics)
  {
    case CARDANO_UPLC_SEMANTICS_D:
    case CARDANO_UPLC_SEMANTICS_E:
    {
      result = true;
      break;
    }
    case CARDANO_UPLC_SEMANTICS_A:
    case CARDANO_UPLC_SEMANTICS_B:
    case CARDANO_UPLC_SEMANTICS_C:
    {
      result = false;
      break;
    }
    default:
    {
      break;
    }
  }

  return result;
}

bool
cardano_uplc_semantics_cons_byte_string_range_checks(cardano_uplc_builtin_semantics_t semantics)
{
  bool result = false;

  switch (semantics)
  {
    case CARDANO_UPLC_SEMANTICS_C:
    case CARDANO_UPLC_SEMANTICS_E:
    {
      result = true;
      break;
    }
    case CARDANO_UPLC_SEMANTICS_A:
    case CARDANO_UPLC_SEMANTICS_B:
    case CARDANO_UPLC_SEMANTICS_D:
    {
      result = false;
      break;
    }
    default:
    {
      break;
    }
  }

  return result;
}
