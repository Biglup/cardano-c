/**
 * \file json_writer_common.c
 *
 * \author angel.castillo
 * \date   Dec 09, 2024
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

/* INCLUDES ******************************************************************/

#include "json_writer_common.h"

#include <assert.h>
#include <string.h>

/* FUNCTIONS *****************************************************************/

void
cardano_json_writer_set_message_if_error(
  cardano_json_writer_t* writer,
  const cardano_error_t  error,
  const char*            message)
{
  assert(writer != NULL);
  assert(message != NULL);

  if (writer->last_error != CARDANO_SUCCESS)
  {
    return;
  }

  writer->last_error = error;
  cardano_object_set_last_error(&writer->base, message);
}