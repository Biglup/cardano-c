/**
 * \file cbor_reader_collections.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_CBOR_READER_INTERNAL_COLLECTIONS_H
#define BIGLUP_LABS_INCLUDE_CARDANO_CBOR_READER_INTERNAL_COLLECTIONS_H

/* INCLUDES ******************************************************************/

#include <cardano/buffer.h>
#include <cardano/error.h>
#include <stdint.h>

#include "cbor_reader_core.h"

/* DECLARATIONS **************************************************************/

/**
 * \brief This function initiates the process of reading a CBOR string (text or byte string) that is encoded
 * with indefinite length.
 *
 * It checks if the next item in the data stream corresponds to the start of an indefinite-length string of
 * the specified major type.
 *
 * \param[in] reader A pointer to an initialized cardano_cbor_reader_t structure that represents the CBOR
 *                   reader context.
 * \param[in] type The major type of the string to read, which should be either \ref CARDANO_CBOR_MAJOR_TYPE_BYTE_STRING
 *                 or \ref CARDANO_CBOR_MAJOR_TYPE_UTF8_STRING, indicating a byte string or a text string, respectively.
 *
 * \return A cardano_error_t indicating the success or failure of the operation. \ref CARDANO_SUCCESS is
 * returned if the start of an indefinite-length string of the specified type is successfully detected.
 * If the operation fails, an appropriate error code is returned to indicate the reason for failure.
 */
cardano_error_t
_cbor_reader_read_start_indefinite_length_string(cardano_cbor_reader_t* reader, cardano_cbor_major_type_t type);

/**
 * \brief This function concludes the process of reading a CBOR string (text or byte string) that is encoded with
 * indefinite length.
 *
 * It verifies that the next item in the data stream corresponds to the "break" stop code, which marks the end of
 * an indefinite-length string.
 *
 * \param[in] reader A pointer to an initialized cardano_cbor_reader_t structure that represents the
 *                   CBOR reader context.
 * \param[in] type The major type of the string being read, which should be either \ref CARDANO_CBOR_MAJOR_TYPE_BYTE_STRING
 *                 or \ref CARDANO_CBOR_MAJOR_TYPE_UTF8_STRING, indicating a byte string or a text string, respectively.
 *
 * \return A cardano_error_t indicating the success or failure of the operation. \ref CARDANO_SUCCESS
 * is returned if the "break" stop code, marking the end of an indefinite-length string of the specified type,
 * is successfully detected. If the operation fails, an appropriate error code is returned to
 * indicate the reason for failure.
 */
cardano_error_t
_cbor_reader_read_end_indefinite_length_string(cardano_cbor_reader_t* reader, cardano_cbor_major_type_t type);

/**
 * \brief This function is used to read data that is encoded in CBOR format with an indefinite length.
 * It concatenates chunks of data until the "break" stop code is encountered, which marks the end
 * of the indefinite length encoding. The concatenated data is stored in a newly allocated buffer.
 *
 * \param[in]  reader           A pointer to an initialized cardano_cbor_reader_t structure that represent
 *                              the CBOR reader context.
 * \param[out] buffer           A pointer to a pointer to a cardano_buffer_t structure where the concatenated
 *                              data will be stored.
 * \param[out] encoding_length  A pointer to a size_t variable where the length of the concatenated
 *                              data will be stored.
 *
 * \return A cardano_error_t indicating the success or failure of the operation. \ref CARDANO_SUCCESS is
 * returned if the data is successfully read and concatenated. If the operation fails, an appropriate
 * error code is returned to indicate the reason for failure.
 */
cardano_error_t
_cbor_reader_read_indefinite_length_concatenated(cardano_cbor_reader_t* reader, cardano_buffer_t** buffer, size_t* encoding_length);

/**
 * \brief Reads the start of a CBOR array and returns its size.
 *
 * This function reads the start of an array (major type 4) from the CBOR stream and returns the size of
 * the array. If the array is of indefinite length, the function will set the size to -1.
 *
 * \param[in] reader A pointer to the cardano_cbor_reader_t structure that represents the CBOR stream from
 *                   which the array start is to be read.
 * \param[out] size A pointer to an int64_t variable where the size of the array will be stored. If the
 *                  array is of indefinite length, the value will be set to -1.
 *
 * \return A cardano_error_t indicating the success or failure of the operation. \ref CARDANO_SUCCESS is
 * returned if the data is successfully read. If the operation fails, an appropriate error code is returned
 * to indicate the reason for failure.
 */
cardano_error_t
_cbor_reader_read_start_array(cardano_cbor_reader_t* reader, int64_t* size);

/**
 * \brief Reads the end of a CBOR array from the stream.
 *
 * This function is used to read the end of an indefinite-length array (major type 4) from the CBOR stream.
 * It checks for the presence of the "break" stop code that indicates the end of the array. It is only applicable
 * for indefinite-length arrays.
 *
 * \param[in] reader A pointer to the cardano_cbor_reader_t structure that represents the CBOR stream
 * from which the end of the array is to be read.
 *
 * \return A cardano_error_t indicating the success or failure of the operation. \ref CARDANO_SUCCESS is
 * returned if the data is successfully read. If the operation fails, an appropriate error code is returned
 * to indicate the reason for failure.
 */
cardano_error_t
_cbor_reader_read_end_array(cardano_cbor_reader_t* reader);

/**
 * \brief Reads the start of a CBOR map and returns its size.
 *
 * This function reads the start of an map (major type 5) from the CBOR stream and returns the size of the map.
 * If the map is of indefinite length, the function will set the size to -1.
 *
 * \param[in] reader A pointer to the cardano_cbor_reader_t structure that represents the CBOR stream from
 *                   which the map start is to be read.
 * \param[out] size A pointer to an int64_t variable where the size of the map will be stored. If the
 *                  map is of indefinite length, the value will be set to -1.
 *
 * \return A cardano_error_t indicating the success or failure of the operation. \ref CARDANO_SUCCESS is
 * returned if the data is successfully read. If the operation fails, an appropriate error code is returned
 * to indicate the reason for failure.
 */
cardano_error_t
_cbor_reader_read_start_map(cardano_cbor_reader_t* reader, int64_t* size);

/**
 * \brief Reads the end of a CBOR map from the stream.
 *
 * This function is used to read the end of an indefinite-length map (major type 5) from the CBOR stream.
 * It checks for the presence of the "break" stop code that indicates the end of the map. It is only applicable
 * for indefinite-length maps.
 *
 * \param[in] reader A pointer to the cardano_cbor_reader_t structure that represents the CBOR stream
 * from which the end of the map is to be read.
 *
 * \return A cardano_error_t indicating the success or failure of the operation. \ref CARDANO_SUCCESS is
 * returned if the data is successfully read. If the operation fails, an appropriate error code is returned
 * to indicate the reason for failure.
 */
cardano_error_t
_cbor_reader_read_end_map(cardano_cbor_reader_t* reader);

/**
 * \brief Reads a string (byte or text) from the CBOR stream.
 *
 * This function reads a string, either a byte string (major type 2) or a text string (major type 3),
 * from the CBOR stream. It handles both definite and indefinite length strings. For indefinite
 * length strings, it concatenates all the chunks into a single buffer.
 *
 * \param[in] reader A pointer to the cardano_cbor_reader_t structure that represents the CBOR stream
 *                   from which the string is to be read.
 * \param[in] type The major type of the string to be read. This must be either CARDANO_CBOR_MAJOR_TYPE_BYTE_STRING (2)
 *                 for byte strings or CARDANO_CBOR_MAJOR_TYPE_UTF8_STRING (3) for text strings.
 * \param[out] byte_string A pointer to a pointer of a cardano_buffer_t structure where the read string will
 *                         be stored. The buffer will contain the string data if the function succeeds.
 *                         The caller is responsible for releasing the buffer using \ref cardano_buffer_unref
 *                         when it is no longer needed.
 *
 * \return A cardano_error_t indicating the success or failure of the operation. \ref CARDANO_SUCCESS is
 * returned if the data is successfully read. If the operation fails, an appropriate error code is returned
 * to indicate the reason for failure.
 */
cardano_error_t
_cbor_reader_read_string(cardano_cbor_reader_t* reader, cardano_cbor_major_type_t type, cardano_buffer_t** byte_string);

#endif // BIGLUP_LABS_INCLUDE_CARDANO_CBOR_READER_INTERNAL_COLLECTIONS_H