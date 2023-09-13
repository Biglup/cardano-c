/**
 * \file cbor_writer.h
 *
 * \author angel.castillo
 * \date   Sep 12, 2023
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

#ifndef CARDANO_CBOR_WRITER_H
#define CARDANO_CBOR_WRITER_H

/* INCLUDES ******************************************************************/

#include <cardano/cbor/cbor_tag.h>
#include <cardano/error.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * A simple writer for Concise Binary Object Representation (CBOR) encoded data.
 */
typedef struct cardano_cbor_writer_t cardano_cbor_writer_t;

/**
 * \brief Creates a new instance of a cardano_cbor_writer_t object and sets its properties.
 *
 * \return A strong reference to the new cardano_cbor_writer_t object. The caller must call
 * \ref cardano_cbor_writer_unref to dispose of the object.
 */
cardano_cbor_writer_t* cardano_cbor_writer_new();

/**
 * \brief Decreases the reference count of the cardano_cbor_writer_t object. When its reference count drops
 * to 0, the object is finalized (i.e. its memory is freed).
 */
void cardano_cbor_writer_unref(cardano_cbor_writer_t*);

/**
 * \brief Increases the reference count of the cardano_cbor_writer_t object.
 */
void cardano_cbor_writer_ref(cardano_cbor_writer_t*);

/**
 * \brief Writes the provided value as a tagged bignum encoding, as described in RFC7049 section 2.4.2.
 *
 * \param value The value to write.
 */
cardano_error_t cardano_cbor_writer_write_big_Integer(cardano_cbor_writer_t* writer, uint64_t value);

/**
 * \brief Writes a boolean value (major type 7).
 *
 * \param value The value to write.
 */
cardano_error_t cardano_cbor_writer_write_bool(cardano_cbor_writer_t* writer, bool value);

/**
 * \brief Writes a buffer as a byte string encoding (major type 2).
 *
 * \param value The value to write.
 */
cardano_error_t cardano_cbor_writer_write_byte_string(cardano_cbor_writer_t* writer, byte_t* data, size_t size);

/**
 * \brief Writes the next data item as a UTF-8 text string (major type 3).
 *
 * \param value The string.
 */
cardano_error_t cardano_cbor_writer_write_text_string(cardano_cbor_writer_t* writer, const char* data, size_t size);

/**
 * \brief Writes a single CBOR data item which has already been encoded.
 *
 * \param value The value to write.
 */
cardano_error_t cardano_cbor_writer_write_encoded(cardano_cbor_writer_t* writer, const char* data, size_t size);

/**
 * \brief Writes the start of a definite or indefinite-length array (major type 4).
 *
 * \param length The length of the definite-length array, or undefined for an indefinite-length array.
 */
cardano_error_t cardano_cbor_writer_start_array(cardano_cbor_writer_t* writer, int32_t size);

/**
 * \brief Writes the end of an array (major type 4).
 */
cardano_error_t cardano_cbor_writer_end_array(cardano_cbor_writer_t* writer);

/**
 * \brief Writes the start of a definite or indefinite-length map (major type 5).
 *
 * \param length The length of the definite-length map, or null for an indefinite-length map.
 */
cardano_error_t cardano_cbor_writer_start_map(cardano_cbor_writer_t* writer, int32_t size);

/**
 * \brief Writes the end of a map (major type 5).
 */
cardano_error_t cardano_cbor_writer_end_map(cardano_cbor_writer_t* writer);

/**
 * \brief Writes a value as a signed integer encoding (major types 0,1)
 *
 * \param value The value to write.
 */
cardano_error_t cardano_cbor_writer_int(cardano_cbor_writer_t* writer, uint32_t value);

/**
 * \brief Writes a value as a signed integer encoding (major types 0,1)
 *
 * \param value The value to write.
 */
cardano_error_t cardano_cbor_writer_negative_int(cardano_cbor_writer_t* writer, int32_t value);

/**
 * \brief Writes a null value (major type 7).
 */
cardano_error_t cardano_cbor_writer_null(cardano_cbor_writer_t* writer);

/**
 * \brief Writes an undefined value.
 */
cardano_error_t cardano_cbor_writer_undefined(cardano_cbor_writer_t* writer);

/**
 * \brief Assign a semantic tag (major type 6) to the next data item.
 *
 * \param tag semantic tag.
 */
cardano_error_t cardano_cbor_writer_tag(cardano_cbor_writer_t* writer, cbor_tag_t tag);

/**
 * \brief Returns a new array containing the encoded value.
 */
cardano_error_t cardano_cbor_writer_encode(cardano_cbor_writer_t* writer, byte_t* data, size_t size);

/**
 * \brief Resets the writer to have no data.
 */
cardano_error_t reset(cardano_cbor_writer_t* writer);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // CARDANO_CBOR_WRITER_H