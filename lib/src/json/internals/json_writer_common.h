/**
 * \file json_writer_common.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_JSON_WRITER_COMMON_H
#define BIGLUP_LABS_INCLUDE_CARDANO_JSON_WRITER_COMMON_H

/* INCLUDES ******************************************************************/

#include "../../collections/array.h"
#include <cardano/buffer.h>
#include <cardano/json/json_context.h>
#include <cardano/json/json_format.h>
#include <cardano/json/json_object_type.h>
#include <cardano/object.h>
#include <cardano/typedefs.h>

#include "../../config.h"

/* CONSTANTS *****************************************************************/

static const byte_t* QUOTES       = (const byte_t*)"\"";    // cppcheck-suppress misra-c2012-8.9
static const byte_t* COLON        = (const byte_t*)":";     // cppcheck-suppress misra-c2012-8.9
static const byte_t* COMMA        = (const byte_t*)",";     // cppcheck-suppress misra-c2012-8.9
static const byte_t* OPEN_ARRAY   = (const byte_t*)"[";     // cppcheck-suppress misra-c2012-8.9
static const byte_t* CLOSE_ARRAY  = (const byte_t*)"]";     // cppcheck-suppress misra-c2012-8.9
static const byte_t* OPEN_OBJECT  = (const byte_t*)"{";     // cppcheck-suppress misra-c2012-8.9
static const byte_t* CLOSE_OBJECT = (const byte_t*)"}";     // cppcheck-suppress misra-c2012-8.9
static const byte_t* TRUE         = (const byte_t*)"true";  // cppcheck-suppress misra-c2012-8.9
static const byte_t* FALSE        = (const byte_t*)"false"; // cppcheck-suppress misra-c2012-8.9
static const byte_t* NULL_VALUE   = (const byte_t*)"null";  // cppcheck-suppress misra-c2012-8.9
static const byte_t* NEW_LINE     = (const byte_t*)"\n";    // cppcheck-suppress misra-c2012-8.9
static const byte_t* SPACE        = (const byte_t*)" ";     // cppcheck-suppress misra-c2012-8.9
static const byte_t* ESCAPE       = (const byte_t*)"\\";    // cppcheck-suppress misra-c2012-8.9

/* STRUCTURES ****************************************************************/

/**
 * \brief Structure to represent the current state of a JSON context.
 */
typedef struct
{
    cardano_json_context_t context;      /**< Current context (object, array, root). */
    size_t                 item_count;   /**< Number of items (properties or elements) in the current context. */
    bool                   expect_value; /**< Indicates whether the next write operation should expect a value. */
} cardano_json_stack_frame_t;

/**
 * \brief Provides a API for forward-only, non-cached writing of UTF-8 encoded JSON text.
 */
typedef struct cardano_json_writer_t
{
    cardano_object_t           base;
    cardano_buffer_t*          buffer;
    cardano_error_t            last_error;
    size_t                     depth;
    cardano_json_format_t      format;
    cardano_json_stack_frame_t current_frame[LIB_CARDANO_C_MAX_JSON_DEPTH];
} cardano_json_writer_t;

/* FUNCTIONS *****************************************************************/

/**
 * \brief Sets the last error message if an error occurred.
 *
 * \param writer The JSON writer instance.
 * \param error The error code.
 * \param message The error message.
 */
void
cardano_json_writer_set_message_if_error(cardano_json_writer_t* writer, cardano_error_t error, const char* message);

#endif // BIGLUP_LABS_INCLUDE_CARDANO_JSON_WRITER_COMMON_H