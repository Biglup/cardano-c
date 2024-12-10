/**
 * \file json_object_type.h
 *
 * \author angel.castillo
 * \date   Dec 07, 2024
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_JSON_OBJECT_TYPE_H
#define BIGLUP_LABS_INCLUDE_CARDANO_JSON_OBJECT_TYPE_H

/* INCLUDES ******************************************************************/

#include <cardano/export.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Enumerates the possible types of a JSON object.
 *
 * This enumeration defines the various types a JSON object can represent.
 */
typedef enum
{
  /**
   * \brief Represents a JSON object (key-value pairs).
   */
  CARDANO_JSON_OBJECT_TYPE_OBJECT,

  /**
   * \brief Represents a JSON array (ordered list).
   */
  CARDANO_JSON_OBJECT_TYPE_ARRAY,

  /**
   * \brief Represents a JSON string.
   */
  CARDANO_JSON_OBJECT_TYPE_STRING,

  /**
   * \brief Represents a JSON number (integer or floating-point).
   */
  CARDANO_JSON_OBJECT_TYPE_NUMBER,

  /**
   * \brief Represents a JSON boolean (`true` or `false`).
   */
  CARDANO_JSON_OBJECT_TYPE_BOOLEAN,

  /**
   * \brief Represents a JSON null value.
   */
  CARDANO_JSON_OBJECT_TYPE_NULL
} cardano_json_object_type_t;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_JSON_OBJECT_TYPE_H