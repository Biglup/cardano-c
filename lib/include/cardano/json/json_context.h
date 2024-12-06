/**
 * \file json_context.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_JSON_CONTEXT_H
#define BIGLUP_LABS_INCLUDE_CARDANO_JSON_CONTEXT_H

/* INCLUDES ******************************************************************/

#include <cardano/export.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Enum representing the current context of the JSON writer.
 *
 * This enum defines the possible states of the JSON writer, indicating
 * whether it is at the root level, inside an object, or inside an array.
 */
typedef enum
{
  /**
   * \brief The writer is at the root level (no context set).
   */
  CARDANO_JSON_CONTEXT_ROOT = 0,

  /**
   * \brief The writer is inside an object context.
   */
  CARDANO_JSON_CONTEXT_OBJECT = 1,

  /**
   * \brief The writer is inside an array context.
   */
  CARDANO_JSON_CONTEXT_ARRAY = 2

} cardano_json_context_t;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_JSON_CONTEXT_H