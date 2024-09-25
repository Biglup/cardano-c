/**
 * \file string_safe.h
 *
 * \author luisd.bianchi
 * \date   Apr 28, 2024
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_STRING_SAFE_H
#define BIGLUP_LABS_INCLUDE_CARDANO_STRING_SAFE_H

/* INCLUDES ******************************************************************/

#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Safe version of memcpy that copies up to buffer size into the destination buffer.
 *
 * \param dest Destination buffer.
 * \param dest_size Size of the destination buffer.
 * \param src Source buffer.
 * \param src_size Number of bytes to copy.
 */
void
cardano_safe_memcpy(void* dest, size_t dest_size, const void* src, size_t src_size);

/**
 * \brief Safe version of strlen that limits the maximum number of characters to inspect.
 *
 * \param str The string to measure.
 * \param max_length Maximum number of characters to inspect.
 *
 * \return Length of the string or \c max_length if the string exceeds max_length.
 */
size_t
cardano_safe_strlen(const char* str, size_t max_length);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_STRING_SAFE_H
