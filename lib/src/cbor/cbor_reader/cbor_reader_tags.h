/**
 * \file cbor_reader_tags.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_CBOR_READER_INTERNAL_TAGS_H
#define BIGLUP_LABS_INCLUDE_CARDANO_CBOR_READER_INTERNAL_TAGS_H

/* INCLUDES ******************************************************************/

#include "cbor_reader_core.h"

#include <cardano/cbor/cbor_tag.h>
#include <cardano/error.h>

/* DECLARATIONS **************************************************************/

/**
 * \brief Reads the next CBOR tag from the stream and advances the reader's position.
 *
 * This function reads the next data item in the CBOR stream, expecting it to be a tag (major type 6).
 * It retrieves the tag's value and advances the reader's position past the tag. This operation consumes
 * the tag, making it suitable for scenarios where the tag's value is required for further processing,
 * and the stream needs to be advanced to the next item.
 *
 * \param[in] reader A pointer to the \ref cardano_cbor_reader_t instance representing the CBOR stream
 *                   from which the tag is to be read. The reader must have been previously initialized
 *                   and positioned correctly within the CBOR data stream.
 * \param[out] tag A pointer to a `cardano_cbor_tag_t` variable where the read tag value will be stored. This
 *                 variable is populated with the tag's value if the operation is successful.
 *
 * \return A \ref cardano_error_t indicating the result of the read operation. \ref CARDANO_SUCCESS is
 * returned if a tag is successfully read from the stream. If the operation fails due to reasons such
 * as incorrect stream positioning, unexpected data format, or if the next item in the stream is not a tag,
 * an appropriate error code will be returned to indicate the failure reason.
 */
cardano_error_t
_cbor_reader_read_tag(cardano_cbor_reader_t* reader, cardano_cbor_tag_t* tag);

/**
 * \brief Peeks at the next CBOR tag from the stream without advancing the reader's position.
 *
 * This function examines the next data item in the CBOR stream, expecting it to be a tag (major type 6).
 * It retrieves the tag's value without advancing the reader's position in the stream.
 *
 * \param[in] reader A pointer to the \ref cardano_cbor_reader_t instance representing the CBOR
 *                   stream from which the tag is to be peeked.
 * \param[out] tag A pointer to a `cardano_cbor_tag_t` variable where the peeked tag value will be stored.
 *
 * \return A \ref cardano_error_t indicating the result of the peek operation. \ref CARDANO_SUCCESS is
 * returned if a tag is successfully peeked from the stream. If the operation fails due to
 * reasons such as incorrect stream positioning, unexpected data format, or if the
 * next item in the stream is not a tag, an appropriate error code will be returned to
 * indicate the failure reason.
 */
cardano_error_t
_cbor_reader_peek_tag(cardano_cbor_reader_t* reader, cardano_cbor_tag_t* tag);

#endif // BIGLUP_LABS_INCLUDE_CARDANO_CBOR_READER_INTERNAL_TAGS_H