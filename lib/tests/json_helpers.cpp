/**
 * \file json_helpers.cpp
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

/* INCLUDES ******************************************************************/

#include "./json_helpers.h"
#include <gmock/gmock.h>

/* DEFINITIONS ***************************************************************/

char*
encode_json(cardano_json_writer_t* writer)
{
  const size_t json_size = cardano_json_writer_get_encoded_size(writer);
  char*        json_str  = static_cast<char*>(malloc(json_size ? json_size : 1));

  if (json_size != 0)
  {
    EXPECT_EQ(cardano_json_writer_encode(writer, json_str, json_size), CARDANO_SUCCESS);
  }
  else
  {
    json_str[0] = '\0';
  }

  return json_str;
}