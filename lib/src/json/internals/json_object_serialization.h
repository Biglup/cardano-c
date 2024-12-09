/**
 * \file json_object_serialization.h
 *
 * \author angel.castillo
 * \date   Nov 09, 2024
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_JSON_OBJECT_SERIALIZATION_H
#define BIGLUP_LABS_INCLUDE_CARDANO_JSON_OBJECT_SERIALIZATION_H

/* INCLUDES ******************************************************************/

#include <cardano/json/json_object.h>
#include <cardano/json/json_writer.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

cardano_error_t
cardano_write_json_object(cardano_json_writer_t* writer, const cardano_json_object_t* object);

void
cardano_write_json_object_type_object(cardano_json_writer_t* writer, const cardano_json_object_t* object);

void
cardano_write_json_object_type_array(cardano_json_writer_t* writer, const cardano_json_object_t* array);

void
cardano_write_json_object_type_string(cardano_json_writer_t* writer, const cardano_json_object_t* string_obj);

void
cardano_write_json_object_type_number(cardano_json_writer_t* writer, const cardano_json_object_t* number_obj);

void
cardano_write_json_object_type_boolean(cardano_json_writer_t* writer, const cardano_json_object_t* bool_obj);

#endif // BIGLUP_LABS_INCLUDE_CARDANO_JSON_OBJECT_SERIALIZATION_H