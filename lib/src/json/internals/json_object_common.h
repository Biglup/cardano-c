/**
 * \file json_object_common.h
 *
 * \author angel.castillo
 * \date   Nov 08, 2024
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_JSON_OBJECT_COMMON_H
#define BIGLUP_LABS_INCLUDE_CARDANO_JSON_OBJECT_COMMON_H

/* INCLUDES ******************************************************************/

#include "../../collections/array.h"
#include <cardano/buffer.h>
#include <cardano/json/json_object_type.h>
#include <cardano/object.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

/**
 * \brief Represents a JSON value in the Cardano library.
 *
 * This structure encapsulates a JSON value, which can be an object, array, string, number,
 * boolean, or null. It provides a flexible representation of JSON data and includes metadata
 * for efficiently accessing the type and value of the JSON element.
 *
 * The structure supports the following JSON types:
 * - Object: A collection of key-value pairs.
 * - Array: An ordered collection of values.
 * - String: A UTF-8 encoded sequence of characters.
 * - Number: A numeric value that can be integer or floating-point.
 * - Boolean: A true or false value.
 * - Null: Represents a JSON null value.
 */
typedef struct cardano_json_object_t
{
    cardano_object_t           base;               /**< Reference counting and error handling base. */
    cardano_json_object_type_t type;               /**< Type of the JSON value. */
    cardano_array_t*           pairs;              /**< Array of key-value pairs for objects. */
    cardano_array_t*           array;              /**< Array of elements for JSON arrays. */
    cardano_buffer_t*          string;             /**< Buffer holding the UTF-8 string value. */
    bool                       is_real;            /**< Indicates if the number is a floating-point value. */
    bool                       is_negative;        /**< Indicates if the number is negative. */
    int64_t                    int_value;          /**< Signed integer value for JSON numbers. */
    uint64_t                   uint_value;         /**< Unsigned integer value for JSON numbers. */
    double                     double_value;       /**< Floating-point value for JSON numbers. */
    bool                       bool_value;         /**< Boolean value for JSON booleans. */
    char*                      json_string;        /**< String representation of the JSON value. */
    size_t                     json_string_length; /**< Length of the string representation. */

} cardano_json_object_t;

/**
 * \brief Represents a key-value pair in a JSON object.
 *
 * This structure encapsulates a key-value pair within a JSON object. It provides
 * fields to store the key and the associated value, enabling structured access to
 * JSON object properties. The `key` is represented as a UTF-8 encoded string, and
 * the `value` is a pointer to another \ref cardano_json_object_t, allowing nested
 * JSON structures.
 */
typedef struct cardano_json_kvp_t
{
    cardano_object_t       base;
    cardano_buffer_t*      key;
    cardano_json_object_t* value;
} cardano_json_kvp_t;

/* FUNCTIONS *****************************************************************/

/**
 * \brief Creates a new, empty JSON object.
 *
 * This function allocates and initializes a new \ref cardano_json_object_t
 * instance, representing an empty JSON value. The created object can later be
 * populated with data such as arrays, objects, strings, numbers, booleans, or null.
 *
 * \return A pointer to the newly created \ref cardano_json_object_t instance, or \c NULL if
 *         the allocation fails due to insufficient memory.
 */
cardano_json_object_t* cardano_json_object_new(void);

/**
 * \brief Creates a new, empty key-value pair for a JSON object.
 *
 * This function allocates and initializes a new \ref cardano_json_kvp_t
 * instance, representing a single key-value pair in a JSON object.
 *
 * \return A pointer to the newly created \ref cardano_json_kvp_t instance, or \c NULL if
 *         the allocation fails due to insufficient memory.
 */
cardano_json_kvp_t* cardano_json_kvp_new(void);

#endif // BIGLUP_LABS_INCLUDE_CARDANO_JSON_OBJECT_COMMON_H