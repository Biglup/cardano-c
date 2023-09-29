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
#include <cardano/export.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief A simple writer for Concise Binary Object Representation (CBOR) encoded data.
 */
typedef struct cardano_cbor_writer_t cardano_cbor_writer_t;

/**
 * \brief Creates a new instance of a cardano_cbor_writer_t object and sets its properties.
 *
 * \return A strong reference to the new cardano_cbor_writer_t object. The caller must call
 * \ref cardano_cbor_writer_unref to dispose of the object.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_cbor_writer_t* cardano_cbor_writer_new(void);

/**
 * \brief Decreases the reference count of the cardano_cbor_writer_t object. When its reference count drops
 * to 0, the object is finalized (i.e. its memory is freed).
 *
 * \param cbor_writer A pointer to the cbor writer object reference.
 */
CARDANO_EXPORT void cardano_cbor_writer_unref(cardano_cbor_writer_t** cbor_writer);

/**
 * \brief Increases the reference count of the cardano_cbor_writer_t object.
 *
 * \param cbor_writer the cbor writer object.
 */
CARDANO_EXPORT void cardano_cbor_writer_ref(cardano_cbor_writer_t* cbor_writer);

/**
 * \brief Get the cbor_writer's reference count
 *
 * \rst
 * .. warning:: This does *not* account for transitive references.
 * \endrst
 *
 * \param cbor_writer the cbor writer object.
 * \return the reference count
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_cbor_writer_refcount(const cardano_cbor_writer_t* cbor_writer);

/**
 * \brief Provides CPP-like move construct
 *
 * Decreases the reference count by one, but does not deallocate the item even
 * if its refcount reaches zero. This is useful for passing intermediate values
 * to functions that increase reference count. Should only be used with
 * functions that `ref` their arguments.
 *
 * \rst
 * .. warning:: If the object is moved without correctly increasing the reference
 *  count afterwards, the memory will be leaked.
 * \endrst
 *
 * \param object Reference to an object
 * \return the object with reference count decreased by one
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_cbor_writer_t* cardano_cbor_writer_move(cardano_cbor_writer_t* cbor_writer);

/**
 * \brief Writes the provided value as a tagged bignum encoding, as described in RFC7049 section 2.4.2.
 *
 * \param writer[in] The CBOR writer instance.
 * \param value[in]  The value to write.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_cbor_writer_write_big_integer(cardano_cbor_writer_t* writer, uint64_t value);

/**
 * \brief Writes a boolean value (major type 7).
 *
 * \param value The value to write.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_cbor_writer_write_bool(cardano_cbor_writer_t* writer, bool value);

/**
 * \brief Writes a buffer as a byte string encoding (major type 2).
 *
 * \param value The value to write.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_cbor_writer_write_byte_string(cardano_cbor_writer_t* writer, byte_t* data, size_t size);

/**
 * \brief Writes the next data item as a UTF-8 text string (major type 3).
 *
 * \param value The string.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_cbor_writer_write_text_string(cardano_cbor_writer_t* writer, const char* data, size_t size);

/**
 * \brief Writes a single CBOR data item which has already been encoded.
 *
 * \param value The value to write.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_cbor_writer_write_encoded(cardano_cbor_writer_t* writer, byte_t* data, size_t size);

/**
 * \brief Writes the start of a definite or indefinite-length array (major type 4).
 *
 * \param length The length of the definite-length array, or 0 for an indefinite-length array.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_cbor_writer_write_start_array(cardano_cbor_writer_t* writer, size_t size);

/**
 * \brief Writes the end of an array (major type 4).
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_cbor_writer_write_end_array(cardano_cbor_writer_t* writer);

/**
 * \brief Writes the start of a definite or indefinite-length map (major type 5).
 *
 * \param length The length of the definite-length map, or 0 for an indefinite-length map.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_cbor_writer_write_start_map(cardano_cbor_writer_t* writer, size_t size);

/**
 * \brief Writes the end of a map (major type 5).
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_cbor_writer_write_end_map(cardano_cbor_writer_t* writer);

/**
 * \brief Writes a value as a signed integer encoding (major types 0,1)
 *
 * \param value The value to write.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_cbor_writer_write_unsigned_int(cardano_cbor_writer_t* writer, uint64_t value);

/**
 * \brief Writes a value as a signed integer encoding (major types 0,1)
 *
 * \param value The value to write.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_cbor_writer_write_signed_int(cardano_cbor_writer_t* writer, int64_t value);

/**
 * \brief Writes a null value (major type 7).
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_cbor_writer_write_null(cardano_cbor_writer_t* writer);

/**
 * \brief Writes an undefined value.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_cbor_writer_write_undefined(cardano_cbor_writer_t* writer);

/**
 * \brief Assign a semantic tag (major type 6) to the next data item.
 *
 * \param tag semantic tag.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_cbor_writer_write_tag(cardano_cbor_writer_t* writer, cbor_tag_t tag);

/**
 * \brief Returns a new array containing the encoded value.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_cbor_writer_encode(cardano_cbor_writer_t* writer, byte_t* data, size_t size, size_t* written);

/**
 * \brief Creates a new hex string with the writer encoded data.
 *
 * \param writer[in] Source writer.
 *
 * \return The newly null terminated char string with the hex representation or NULL on memory allocation failure.
 * The caller assumes ownership of the returned char* string and is responsible for its lifecycle.
 * It must be freed when no longer in use.
 */
CARDANO_NODISCARD
CARDANO_EXPORT char* cardano_cbor_writer_encode_hex(cardano_cbor_writer_t* writer);

/**
 * \brief Resets the writer to have no data.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_cbor_writer_reset(cardano_cbor_writer_t* writer);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // CARDANO_CBOR_WRITER_H