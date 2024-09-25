/**
 * \file cbor_initial_byte.h
 *
 * \author angel.castillo
 * \date   Sep 29, 2023
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_CBOR_INITIAL_BYTE_H
#define BIGLUP_LABS_INCLUDE_CARDANO_CBOR_INITIAL_BYTE_H

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
 * \brief Creates a new CBOR initial byte by packing the major type and additional information.
 *
 * Combines a CBOR major type and additional information into a single initial byte according to the
 * CBOR encoding rules. This initial byte is used at the start of a CBOR encoded data item to indicate
 * the type of the item and provide additional context about its size or value.
 *
 * \param[in] major_type Major type of the CBOR data item, indicating the broad category of the data
 *                       (e.g., unsigned integer, byte string, array).
 * \param[in] additional_info Additional information providing further details about the data item, such
 *                            as its size or special values like `false`, `true`, `null`.
 *
 * \return The packed CBOR initial byte combining both the major type and additional information.
 */
CARDANO_NODISCARD
CARDANO_EXPORT byte_t cardano_cbor_initial_byte_pack(cardano_cbor_major_type_t major_type, cardano_cbor_additional_info_t additional_info);

/**
 * \brief Retrieves the major type from a CBOR initial byte.
 *
 * Extracts the major type from a given CBOR initial byte. The major type defines the high-level
 * data type of a CBOR data item, such as unsigned integer, byte string, array, etc. This function
 * parses the initial byte to identify and return the major type it represents.
 *
 * \param[in] initial_byte The CBOR initial byte from which to extract the major type.
 *
 * \return The extracted major type of the CBOR data item, as defined by the CBOR encoding specification.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_cbor_major_type_t cardano_cbor_initial_byte_get_major_type(byte_t initial_byte);

/**
 * \brief Retrieves the additional information from a CBOR initial byte.
 *
 * Extracts the additional information encoded within a CBOR initial byte. This information provides
 * further details about the data item, such as the length of the content or special values like
 * "undefined" or "null". It complements the major type to fully describe the nature of the CBOR data item.
 *
 * \param[in] initial_byte The CBOR initial byte from which to extract the additional information.
 *
 * \return The extracted additional information, as an enum value of type cardano_cbor_additional_info_t,
 * indicating specific data characteristics or lengths as defined by the CBOR encoding specification.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_cbor_additional_info_t cardano_cbor_initial_byte_get_additional_info(byte_t initial_byte);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_CBOR_INITIAL_BYTE_H
