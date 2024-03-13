/**
 * \file cardano.h
 *
 * \author angel.castillo
 * \date   Mar 13, 2024
 *
 * \section LICENSE
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

#ifndef CARDANO_H
#define CARDANO_H

/* INCLUDES ******************************************************************/

#include <cardano/allocators.h>
#include <cardano/buffer.h>
#include <cardano/cbor/cbor_major_type.h>
#include <cardano/cbor/cbor_reader_state.h>
#include <cardano/cbor/cbor_simple_value.h>
#include <cardano/cbor/cbor_tag.h>
#include <cardano/cbor/cbor_writer.h>
#include <cardano/error.h>
#include <cardano/object.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Retrieves the version of the Cardano C library.
 *
 * This function returns a string representing the version of the Cardano C library. The string
 * is statically allocated and should not be freed by the caller. The version string follows the
 * Semantic Versioning (SemVer) format, which consists of three segments: MAJOR.MINOR.PATCH
 * (e.g., "1.0.3").
 *
 * \return A pointer to a statically allocated string containing the library's version. This string
 * is located in read-only memory and must not be modified or freed by the caller.
 */
CARDANO_EXPORT const char* cardano_get_lib_version(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // CARDANO_H
