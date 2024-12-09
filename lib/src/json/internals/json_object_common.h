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

typedef struct cardano_json_object_t
{
    cardano_object_t           base;
    cardano_json_object_type_t type;
    cardano_array_t*           pairs;
    cardano_array_t*           array;
    cardano_buffer_t*          string;
    bool                       is_real;
    bool                       is_negative;
    int64_t                    int_value;
    uint64_t                   uint_value;
    double                     double_value;
    bool                       bool_value;

} cardano_json_object_t;

typedef struct cardano_json_kvp_t
{
    cardano_object_t       base;
    cardano_buffer_t*      key;
    cardano_json_object_t* value;
} cardano_json_kvp_t;

/* FUNCTIONS *****************************************************************/

cardano_json_object_t* cardano_json_object_new(void);

cardano_json_kvp_t* cardano_json_kvp_new(void);

#endif // BIGLUP_LABS_INCLUDE_CARDANO_JSON_OBJECT_COMMON_H