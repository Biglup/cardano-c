/**
 * \file json_format.h
 *
 * \author angel.castillo
 * \date   Dec 06, 2024
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_JSON_FORMAT_H
#define BIGLUP_LABS_INCLUDE_CARDANO_JSON_FORMAT_H

/* INCLUDES ******************************************************************/

#include <cardano/export.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Enum representing the format of the JSON output.
 *
 * This enum defines the possible formats for the JSON output, indicating
 * whether it should be compact (no extra spaces or line breaks) or pretty
 * (extra spaces and line breaks for readability).
 */
typedef enum
{
  /**
   * \brief Compact JSON format (no extra spaces or line breaks).
   */
  CARDANO_JSON_FORMAT_COMPACT,

  /**
   * \brief Pretty JSON format (extra spaces and line breaks for readability).
   */
  CARDANO_JSON_FORMAT_PRETTY

} cardano_json_format_t;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_JSON_FORMAT_H