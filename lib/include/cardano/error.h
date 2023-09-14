/**
 * \file error.h
 *
 * \author angel.castillo
 * \date   Sep 09, 2023
 *
 * \section LICENSE
 *
 * Copyright 2023 Biglup Labs
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

#ifndef CARDANO_ERROR_H
#define CARDANO_ERROR_H

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Cardano C library error codes.
 */
typedef enum
{
  /**
   * \brief Successful operation.
   */
  CARDANO_SUCCESS = 0,

  /**
   * \brief Generic error.
   */
  CARDANO_ERROR_GENERIC = 1,

  /**
   * \brief Insufficient buffer size.
   */
  CARDANO_INSUFFICIENT_BUFFER_SIZE = 2,

  /**
   * \brief Operation over a null pointer.
   */
  CARDANO_POINTER_IS_NULL = 3,

  /**
   * \brief Memory could not be allocated.
   */
  CARDANO_MEMORY_ALLOCATION_FAILED = 4,

  /* Serialization errors */

  /**
   * \brief The serialization or deserialization process resulted in a loss of precision.
   */
  CARDANO_ERROR_LOSS_OF_PRECISION = 100
} cardano_error_t;

/**
 * \brief Converts error codes to their human readable form.
 *
 * \param error[in] The error code to get the string representation for.
 * \return Human readable form of the given error code.
 */
const char* cardano_error_to_string(cardano_error_t error);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // CARDANO_ERROR_H