/**
 * \file uplc_byte_view.h
 *
 * \author angel.castillo
 * \date   Jun 18, 2026
 *
 * Copyright 2026 Biglup Labs
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_UPLC_BUILTINS_UPLC_BYTE_VIEW_H
#define BIGLUP_LABS_INCLUDE_CARDANO_UPLC_BUILTINS_UPLC_BYTE_VIEW_H

/* INCLUDES ******************************************************************/

#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief A borrowed, read-only view of a constant's byte-string or string payload.
 *
 * The machine stores byte strings and strings as an inline arena \c (data, size)
 * descriptor, so unwrapping one yields a pointer-and-length view straight into arena
 * memory with no allocation or refcount. The view borrows the arena bytes and must
 * not outlive them. For the empty byte string \c data is NULL and \c size is 0.
 */
typedef struct cardano_uplc_byte_view_t
{
  const byte_t* data;
  size_t        size;
} cardano_uplc_byte_view_t;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BIGLUP_LABS_INCLUDE_CARDANO_UPLC_BUILTINS_UPLC_BYTE_VIEW_H */
