/**
 * \file uplc_lang_version.c
 *
 * \author angel.castillo
 * \date   Oct 28, 2026
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

#include "uplc_lang_version.h"

#include <stddef.h>
#include <stdint.h>

/* CONSTANTS *****************************************************************/

/**
 * \brief The number of Plutus languages, V1 through V4.
 */
enum
{
  LANG_VERSION_COUNT = 4
};

/**
 * \brief The number of Plutus Core program versions the ledger accepts.
 */
enum
{
  PLC_VERSION_COUNT = 3
};

/**
 * \brief The protocol major version that enables each Plutus language, indexed by
 *        \ref cardano_uplc_lang_version_t.
 *
 * Alonzo (5) for V1, Vasil (7) for V2, Chang (9) for V3 and Dijkstra (12) for V4.
 */
// cppcheck-suppress misra-c2012-8.9; Reason: file-scope constant data grouped with the module
static const uint64_t LANGUAGE_INTRODUCED_IN[LANG_VERSION_COUNT] = { 5U, 7U, 9U, 12U };

/**
 * \brief A Plutus Core program version and the protocol major version that makes
 *        it available in each Plutus language.
 */
typedef struct
{
    uint32_t major;
    uint32_t minor;
    uint32_t patch;
    uint64_t introduced_in[LANG_VERSION_COUNT];
} plc_version_row_t;

/**
 * \brief The ledger's \c plcVersionsIntroducedIn table, one row per program
 *        version, with the introduction protocol for V1, V2, V3 and V4.
 */
// cppcheck-suppress misra-c2012-8.9; Reason: file-scope constant data grouped with the module
static const plc_version_row_t PLC_VERSIONS[PLC_VERSION_COUNT] = {
  { 1U, 0U, 0U, { 5U, 7U, 9U, 12U } },
  { 1U, 1U, 0U, { 11U, 11U, 9U, 12U } },
  { 1U, 2U, 0U, { 12U, 12U, 12U, 12U } }
};

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Reports whether a language value names one of V1 through V4.
 *
 * \param[in] language The language to check.
 *
 * \return \c true for a valid language, \c false otherwise.
 */
static bool
is_valid_language(const cardano_uplc_lang_version_t language)
{
  return ((int)language >= 0) && ((size_t)language < (size_t)LANG_VERSION_COUNT);
}

/* DEFINITIONS ***************************************************************/

bool
cardano_uplc_lang_version_available(const cardano_uplc_lang_version_t language, const uint64_t protocol_major)
{
  if (!is_valid_language(language))
  {
    return false;
  }

  return protocol_major >= LANGUAGE_INTRODUCED_IN[(size_t)language];
}

bool
cardano_uplc_plc_version_available(
  const cardano_uplc_lang_version_t language,
  const uint64_t                    protocol_major,
  const uint32_t                    major,
  const uint32_t                    minor,
  const uint32_t                    patch)
{
  if (!is_valid_language(language))
  {
    return false;
  }

  for (size_t i = 0U; i < (size_t)PLC_VERSION_COUNT; ++i)
  {
    const plc_version_row_t* row = &PLC_VERSIONS[i];

    if ((row->major == major) && (row->minor == minor) && (row->patch == patch))
    {
      return protocol_major >= row->introduced_in[(size_t)language];
    }
  }

  return false;
}
