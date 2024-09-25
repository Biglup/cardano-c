/**
 * \file cbor_reader_simple_values.h
 *
 * \author luisd.bianchi
 * \date   Mar 14, 2024
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_CBOR_READER_INTERNAL_SIMPLE_VALUES_H
#define BIGLUP_LABS_INCLUDE_CARDANO_CBOR_READER_INTERNAL_SIMPLE_VALUES_H

/* INCLUDES ******************************************************************/

#include <cardano/cbor/cbor_reader.h>
#include <cardano/cbor/cbor_simple_value.h>
#include <cardano/error.h>

/* DECLARATIONS **************************************************************/

/**
 * \brief Reads a boolean value from the CBOR stream.
 *
 * This function reads the next data item from the CBOR stream associated with the reader
 * and interprets it as a boolean value. CBOR represents boolean values as simple values with
 * specific predefined byte values (true: 0xF5, false: 0xF4). This function decodes these representations
 * and stores the boolean result in the provided bool variable.
 *
 * \param[in] reader A pointer to the \ref cardano_cbor_reader_t instance that represents the CBOR
 *                   stream from which the boolean value is to be read.
 * \param[out] value A pointer to a bool variable where the decoded boolean value will be stored.
 *
 * \return A \ref cardano_error_t indicating the result of the read operation. \ref CARDANO_SUCCESS is
 * returned if a boolean value is successfully read from the stream and decoded. If the operation fails
 * due to reasons such as incorrect stream positioning, unexpected data format, or if the next item in the
 * stream is not a boolean value, an appropriate error code will be returned to indicate the failure reason.
 */
cardano_error_t
_cbor_reader_read_boolean(cardano_cbor_reader_t* reader, bool* value);

/**
 * \brief Reads a null value from the CBOR stream.
 *
 * This function reads the next data item from the CBOR stream associated with the reader
 * and checks if it represents a null value. In CBOR, a null value is represented as a simple
 * value with a specific predefined byte value (0xF6). This function ensures that the next item
 * in the stream is indeed a null value.
 *
 * \param[in] reader A pointer to the \ref cardano_cbor_reader_t instance that represents the CBOR
 *                   stream from which the null value is to be read.
 *
 * \return A \ref cardano_error_t indicating the result of the read operation. \ref CARDANO_SUCCESS is
 * returned if a null value is successfully identified in the stream. If the operation fails
 * due to reasons such as incorrect stream positioning, unexpected data format, or if the next item in the
 * stream is not a null value, an appropriate error code will be returned to indicate the failure reason.
 */
cardano_error_t
_cbor_reader_read_null(cardano_cbor_reader_t* reader);

/**
 * \brief Reads a CBOR simple value from the CBOR stream.
 *
 * This function reads the next data item from the CBOR stream associated with the reader
 * and interprets it as a simple value. CBOR simple values are small, non-structured values
 * that include boolean values, null, undefined, and simple error conditions. This function
 * decodes the simple value representation and stores the result in the provided `cardano_cbor_simple_value_t`
 * variable.
 *
 * \param[in] reader A pointer to the \ref cardano_cbor_reader_t instance that represents the CBOR
 *                   stream from which the simple value is to be read.
 * \param[out] value A pointer to a `cardano_cbor_simple_value_t` variable where the decoded simple value will be stored.
 *
 * \return A \ref cardano_error_t indicating the result of the read operation. \ref CARDANO_SUCCESS is
 * returned if a simple value is successfully read from the stream and decoded. If the operation fails
 * due to reasons such as incorrect stream positioning, unexpected data format, or if the next item in the
 * stream is not a simple value, an appropriate error code will be returned to indicate the failure reason.
 */
cardano_error_t
_cbor_reader_read_simple_value(cardano_cbor_reader_t* reader, cardano_cbor_simple_value_t* value);

#endif // BIGLUP_LABS_INCLUDE_CARDANO_CBOR_READER_INTERNAL_SIMPLE_VALUES_H