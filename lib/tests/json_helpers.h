/**
 * \file allocators_helpers.h
 *
 * \author angel.castillo
 * \date   Oct 13, 2025
 *
 * Copyright 2025 Biglup Labs
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_JSON_HELPERS
#define BIGLUP_LABS_INCLUDE_CARDANO_JSON_HELPERS

/* INCLUDES ******************************************************************/

#include <cardano/json/json_writer.h>

/* DECLARATIONS **************************************************************/
/**
 * Encodes the JSON writer content into a string.
 * @param writer The JSON writer to encode.
 * @return A newly allocated string containing the encoded JSON. The caller is responsible for freeing the memory.
 */
char*
encode_json(cardano_json_writer_t* writer);

#endif // BIGLUP_LABS_INCLUDE_CARDANO_JSON_HELPERS
