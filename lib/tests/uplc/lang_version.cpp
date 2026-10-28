/**
 * \file lang_version.cpp
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

#include "../../src/uplc/ast/uplc_lang_version.h"

#include <gmock/gmock.h>

/* STATIC HELPERS ************************************************************/

static const cardano_uplc_lang_version_t LANGUAGES[] = {
  CARDANO_UPLC_LANG_VERSION_V1,
  CARDANO_UPLC_LANG_VERSION_V2,
  CARDANO_UPLC_LANG_VERSION_V3,
  CARDANO_UPLC_LANG_VERSION_V4
};

/* UNIT TESTS ****************************************************************/

TEST(cardano_uplc_lang_version_available, matchesLedgerLanguageIntroducedIn)
{
  const uint64_t introduced_in[] = { 5U, 7U, 9U, 12U };

  for (size_t l = 0U; l < 4U; ++l)
  {
    EXPECT_FALSE(cardano_uplc_lang_version_available(LANGUAGES[l], introduced_in[l] - 1U)) << "language " << l;
    EXPECT_TRUE(cardano_uplc_lang_version_available(LANGUAGES[l], introduced_in[l])) << "language " << l;
    EXPECT_TRUE(cardano_uplc_lang_version_available(LANGUAGES[l], 12U)) << "language " << l;
  }
}

TEST(cardano_uplc_lang_version_available, rejectsAnInvalidLanguage)
{
  EXPECT_FALSE(cardano_uplc_lang_version_available((cardano_uplc_lang_version_t)4, 12U));
  EXPECT_FALSE(cardano_uplc_lang_version_available((cardano_uplc_lang_version_t)-1, 12U));
}

TEST(cardano_uplc_plc_version_available, matchesPlcVersionsIntroducedIn)
{
  struct Row
  {
      uint32_t major;
      uint32_t minor;
      uint32_t patch;
      uint64_t introduced_in[4];
  };

  const Row rows[] = {
    { 1U, 0U, 0U, { 5U, 7U, 9U, 12U } },
    { 1U, 1U, 0U, { 11U, 11U, 9U, 12U } },
    { 1U, 2U, 0U, { 12U, 12U, 12U, 12U } }
  };

  for (const Row& row: rows)
  {
    for (size_t l = 0U; l < 4U; ++l)
    {
      const uint64_t intro = row.introduced_in[l];

      EXPECT_FALSE(cardano_uplc_plc_version_available(LANGUAGES[l], intro - 1U, row.major, row.minor, row.patch))
        << row.major << "." << row.minor << "." << row.patch << " language " << l << " protocol " << (intro - 1U);
      EXPECT_TRUE(cardano_uplc_plc_version_available(LANGUAGES[l], intro, row.major, row.minor, row.patch))
        << row.major << "." << row.minor << "." << row.patch << " language " << l << " protocol " << intro;
      EXPECT_TRUE(cardano_uplc_plc_version_available(LANGUAGES[l], 12U, row.major, row.minor, row.patch))
        << row.major << "." << row.minor << "." << row.patch << " language " << l << " protocol 12";
    }
  }
}

TEST(cardano_uplc_plc_version_available, rejectsVersionsTheLedgerNeverIntroduced)
{
  for (size_t l = 0U; l < 4U; ++l)
  {
    EXPECT_FALSE(cardano_uplc_plc_version_available(LANGUAGES[l], 12U, 0U, 0U, 0U));
    EXPECT_FALSE(cardano_uplc_plc_version_available(LANGUAGES[l], 12U, 1U, 0U, 1U));
    EXPECT_FALSE(cardano_uplc_plc_version_available(LANGUAGES[l], 12U, 1U, 3U, 0U));
    EXPECT_FALSE(cardano_uplc_plc_version_available(LANGUAGES[l], 12U, 2U, 0U, 0U));
  }
}

TEST(cardano_uplc_plc_version_available, rejectsAnInvalidLanguage)
{
  EXPECT_FALSE(cardano_uplc_plc_version_available((cardano_uplc_lang_version_t)4, 12U, 1U, 0U, 0U));
  EXPECT_FALSE(cardano_uplc_plc_version_available((cardano_uplc_lang_version_t)-1, 12U, 1U, 0U, 0U));
}
