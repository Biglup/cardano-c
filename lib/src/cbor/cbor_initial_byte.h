/**
 * \file cbor_initial_byte.h
 *
 * \author angel.castillo
 * \date   Sep 29, 2023
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

#ifndef CARDANO_CBOR_INITIAL_BYTE_H
#define CARDANO_CBOR_INITIAL_BYTE_H

/* INCLUDES ******************************************************************/

#include "cbor_additional_info.h"
#include <cardano/cbor/cbor_major_type.h>
#include <cardano/error.h>
#include <cardano/export.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Creates a new CBOR initial byte.
 *
 * \param major_type[in]        Major type of the CBOR initial byte.
 * \param additional_info[in]   Additional info of the CBOR initial byte.
 *
 * \return The packed CBOR initial byte.
 */
CARDANO_NODISCARD
CARDANO_EXPORT byte_t cardano_cbor_initial_byte_pack(cbor_major_type_t major_type, cbor_additional_info_t additional_info);

/**
 * Gets the major type of the initial byte.
 *
 * \param initial_byte[in] The CBOR initial byte to get the major type from.
 *
 * \return The major type of the initial byte.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cbor_major_type_t cardano_cbor_initial_byte_get_major_type(byte_t initial_byte);

/**
 * Gets the additional info of the initial byte.
 *
 * \param initial_byte[in] The CBOR initial byte to get the additional info from.
 *
 * \return The additional info of the initial byte.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cbor_additional_info_t cardano_cbor_initial_byte_get_additional_info(byte_t initial_byte);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // CARDANO_CBOR_INITIAL_BYTE_H
