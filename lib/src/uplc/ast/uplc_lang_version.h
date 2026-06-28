/**
 * \file uplc_lang_version.h
 *
 * \author angel.castillo
 * \date   Jun 18, 2026
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_UPLC_AST_UPLC_LANG_VERSION_H
#define BIGLUP_LABS_INCLUDE_CARDANO_UPLC_AST_UPLC_LANG_VERSION_H

/* INCLUDES ******************************************************************/

#include <cardano/error.h>
#include <cardano/export.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Plutus language version a builtin first became available in.
 *
 * This is a module-local version enum rather than
 * \ref cardano_plutus_language_version_t because the shared enum stops at V3.
 * Keeping a self-contained enum here lets the builtin metadata name V4 without
 * modifying the shared, public version type. The numeric values match the shared
 * enum (V1=0, V2=1, V3=2) and extend it with V4=3.
 */
typedef enum
{
  CARDANO_UPLC_LANG_VERSION_V1 = 0,
  CARDANO_UPLC_LANG_VERSION_V2 = 1,
  CARDANO_UPLC_LANG_VERSION_V3 = 2,
  CARDANO_UPLC_LANG_VERSION_V4 = 3
} cardano_uplc_lang_version_t;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BIGLUP_LABS_INCLUDE_CARDANO_UPLC_AST_UPLC_LANG_VERSION_H */
