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

/**
 * \brief Returns whether a Plutus language is enabled at a protocol version.
 *
 * Encodes the ledger's \c ledgerLanguageIntroducedIn schedule
 * (plutus-ledger-api/src/PlutusLedgerApi/Common/Versions.hs): V1 from protocol 5,
 * V2 from 7, V3 from 9 and V4 from 12. A script in a language that is not yet
 * enabled cannot be deserialised.
 *
 * \param[in] language The Plutus language of the script.
 * \param[in] protocol_major The major protocol version the script is used under.
 *
 * \return \c true when the language is enabled, \c false otherwise (including for
 *         an invalid language).
 */
bool
cardano_uplc_lang_version_available(cardano_uplc_lang_version_t language, uint64_t protocol_major);

/**
 * \brief Returns whether a Plutus Core program version may be evaluated in a
 *        language at a protocol version.
 *
 * Encodes the ledger's \c plcVersionsIntroducedIn schedule
 * (plutus-ledger-api/src/PlutusLedgerApi/Common/Versions.hs): 1.0.0 is available
 * once the language is enabled, 1.1.0 is available in V1 and V2 from protocol 11
 * and in V3 and V4 once they are enabled, and 1.2.0 is available in every
 * language from protocol 12. Any other version triple is never available.
 *
 * \param[in] language The Plutus language of the script.
 * \param[in] protocol_major The major protocol version the script is evaluated under.
 * \param[in] major The major component of the program version.
 * \param[in] minor The minor component of the program version.
 * \param[in] patch The patch component of the program version.
 *
 * \return \c true when the program version is available, \c false otherwise
 *         (including for an invalid language).
 */
bool
cardano_uplc_plc_version_available(
  cardano_uplc_lang_version_t language,
  uint64_t                    protocol_major,
  uint32_t                    major,
  uint32_t                    minor,
  uint32_t                    patch);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BIGLUP_LABS_INCLUDE_CARDANO_UPLC_AST_UPLC_LANG_VERSION_H */
