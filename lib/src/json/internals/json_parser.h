/**
 * \file json_parser.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_JSON_PARSER_H
#define BIGLUP_LABS_INCLUDE_CARDANO_JSON_PARSER_H

/* INCLUDES ******************************************************************/

#include <cardano/json/json_object.h>
#include <cardano/typedefs.h>

/* STRUCTURES ****************************************************************/

typedef struct
{
    const char* input;
    size_t      length;
    size_t      offset;
    size_t      depth;
    char        last_error[256];
} cardano_json_parse_context_t;

/* DECLARATIONS **************************************************************/

void cardano_skip_whitespace(cardano_json_parse_context_t* ctx);

bool
cardano_has_char(char to_match, const char* begin, const char* end);

bool
cardano_handle_utf8_sequence(cardano_json_parse_context_t* ctx, cardano_buffer_t* buf);
bool
cardano_handle_unicode_sequence(cardano_json_parse_context_t* ctx, cardano_buffer_t* buf);

bool
cardano_handle_escape_sequence(cardano_json_parse_context_t* ctx, cardano_buffer_t* buf);

cardano_json_object_t*
cardano_parse_string_value(cardano_json_parse_context_t* ctx);

cardano_json_object_t*
cardano_parse_number_value(cardano_json_parse_context_t* ctx);

cardano_json_object_t*
cardano_parse_object_value(cardano_json_parse_context_t* ctx);

cardano_json_object_t*
cardano_parse_array_value(cardano_json_parse_context_t* ctx);

cardano_json_object_t*
cardano_parse_literal(
  cardano_json_parse_context_t* ctx,
  const char*                   literal,
  size_t                        literal_size,
  cardano_json_object_type_t    type);

cardano_json_object_t*
cardano_parse_value(cardano_json_parse_context_t* ctx);

#endif // BIGLUP_LABS_INCLUDE_CARDANO_JSON_PARSER_H