/**
 * \file flat_reader.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_UPLC_FLAT_FLAT_READER_H
#define BIGLUP_LABS_INCLUDE_CARDANO_UPLC_FLAT_FLAT_READER_H

/* INCLUDES ******************************************************************/

#include <cardano/buffer.h>
#include <cardano/common/bigint.h>
#include <cardano/error.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief MSB-first bit reader over the flat serialization used for Plutus scripts.
 *
 * Flat is a big-endian bit stream: within each byte the most significant bit is
 * consumed first. The reader tracks the current byte and how many bits of that
 * byte have already been consumed. It does not own \ref buffer; the caller keeps
 * the backing bytes alive for the reader's lifetime.
 */
typedef struct cardano_uplc_flat_reader_t
{
    const byte_t* buffer;
    size_t        size;
    size_t        byte_pos;
    uint8_t       bit_pos;
} cardano_uplc_flat_reader_t;

/**
 * \brief Initializes a reader over \p buffer positioned at the first bit.
 *
 * Sets the cursor to byte 0, bit 0. The reader borrows \p buffer; the caller must
 * keep those bytes valid while the reader is used. A NULL \p buffer is permitted
 * only when \p size is 0, describing an empty stream.
 *
 * \param[out] reader The reader to initialize.
 * \param[in] buffer The flat bytes to read from. May be NULL only if \p size is 0.
 * \param[in] size The number of bytes in \p buffer.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         \p reader is NULL or if \p buffer is NULL while \p size is non-zero.
 */
cardano_error_t
cardano_uplc_flat_reader_init(cardano_uplc_flat_reader_t* reader, const byte_t* buffer, size_t size);

/**
 * \brief Reads one bit, most significant bit of the current byte first.
 *
 * \param[in,out] reader The reader to advance by one bit.
 * \param[out] value Set to 1 if the bit is set, 0 otherwise.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         any argument is NULL, or \ref CARDANO_ERROR_DECODING if the stream is
 *         exhausted.
 */
cardano_error_t
cardano_uplc_flat_reader_bit(cardano_uplc_flat_reader_t* reader, uint8_t* value);

/**
 * \brief Reads \p count bits (0..8) most significant bit first into a byte.
 *
 * The bits are accumulated in stream order, so the first bit read becomes the most
 * significant bit of the result. A \p count of 0 yields 0 and consumes nothing.
 *
 * \param[in,out] reader The reader to advance by \p count bits.
 * \param[in] count The number of bits to read; must be at most 8.
 * \param[out] value The assembled bits, right-aligned.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         any argument is NULL, \ref CARDANO_ERROR_INVALID_ARGUMENT if \p count
 *         exceeds 8, or \ref CARDANO_ERROR_DECODING if the stream is exhausted.
 */
cardano_error_t
cardano_uplc_flat_reader_bits8(cardano_uplc_flat_reader_t* reader, uint8_t count, uint8_t* value);

/**
 * \brief Reads a natural number encoded as 7-bit little-endian continuation groups.
 *
 * Each byte contributes its low 7 bits to the result, least significant group
 * first; the high bit signals that another group follows. A value whose groups
 * would shift past the width of \c size_t is rejected rather than silently
 * truncated.
 *
 * \param[in,out] reader The reader to advance past the encoded word.
 * \param[out] value The decoded natural number.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         any argument is NULL, or \ref CARDANO_ERROR_DECODING if the stream is
 *         exhausted or the encoded value does not fit in \c size_t.
 */
cardano_error_t
cardano_uplc_flat_reader_word(cardano_uplc_flat_reader_t* reader, size_t* value);

/**
 * \brief Consumes zero bits up to and including the next one bit (byte alignment).
 *
 * The flat format pads to a byte boundary with a run of zero bits terminated by a
 * single one bit. This reads that run; it does not require the cursor to already be
 * byte-aligned.
 *
 * \param[in,out] reader The reader to advance past the filler.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         \p reader is NULL, or \ref CARDANO_ERROR_DECODING if the stream ends
 *         before a one bit is seen.
 */
cardano_error_t
cardano_uplc_flat_reader_filler(cardano_uplc_flat_reader_t* reader);

/**
 * \brief Reads a flat bytestring into a newly allocated buffer.
 *
 * Byte-aligns via \ref cardano_uplc_flat_reader_filler, then reads length-prefixed
 * blocks: a one-byte length in 1..255 followed by that many bytes, repeating, until
 * a zero-length block terminates the sequence. The concatenated bytes are returned
 * in \p value, which the caller releases with \ref cardano_buffer_unref. On any
 * failure no buffer is handed back.
 *
 * \param[in,out] reader The reader to advance past the bytestring.
 * \param[out] value On success, the assembled bytes; left untouched on failure.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         any argument is NULL, \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED if the
 *         buffer cannot be grown, or \ref CARDANO_ERROR_DECODING if a block runs
 *         past the end of the stream or the sequence is not terminated.
 */
cardano_error_t
cardano_uplc_flat_reader_bytes(cardano_uplc_flat_reader_t* reader, cardano_buffer_t** value);

/**
 * \brief Reads a natural number of unbounded width as 7-bit little-endian groups.
 *
 * The same continuation scheme as \ref cardano_uplc_flat_reader_word: each byte
 * contributes its low 7 bits, least significant group first, and the high bit
 * signals that another group follows. Unlike \ref cardano_uplc_flat_reader_word the
 * magnitude is accumulated into a \ref cardano_bigint_t, so there is no width
 * ceiling and the value is never rejected for being too large.
 *
 * On success \p value receives a newly created bigint that the caller owns and
 * releases with \ref cardano_bigint_unref. On any failure no bigint is handed back
 * and \p value is left untouched.
 *
 * \param[in,out] reader The reader to advance past the encoded magnitude.
 * \param[out] value On success, the decoded magnitude; left untouched on failure.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         any argument is NULL, \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED if the
 *         bigint cannot be created, or \ref CARDANO_ERROR_DECODING if the stream is
 *         exhausted before the magnitude terminates.
 */
cardano_error_t
cardano_uplc_flat_reader_big_word(cardano_uplc_flat_reader_t* reader, cardano_bigint_t** value);

/**
 * \brief Reads a signed arbitrary-precision integer (magnitude then zig-zag decode).
 *
 * Reads an unsigned magnitude with \ref cardano_uplc_flat_reader_big_word, then
 * applies the zig-zag decode that maps the unsigned encoding back to a signed
 * value: for unsigned \c u the result is <tt>(u >> 1)</tt> when \c u is even and
 * <tt>-((u >> 1) + 1)</tt> when \c u is odd, i.e. <tt>(u >> 1) xor -(u and 1)</tt>
 * over the bigint. A non-negative \c n encodes as \c 2n and a negative \c n as
 * <tt>-2n - 1</tt>.
 *
 * On success \p value receives a newly created bigint that the caller owns and
 * releases with \ref cardano_bigint_unref. On any failure no bigint is handed back
 * and \p value is left untouched.
 *
 * \param[in,out] reader The reader to advance past the encoded integer.
 * \param[out] value On success, the decoded signed integer; left untouched on failure.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         any argument is NULL, \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED if a
 *         bigint cannot be created, or \ref CARDANO_ERROR_DECODING if the stream is
 *         exhausted before the magnitude terminates.
 */
cardano_error_t
cardano_uplc_flat_reader_integer(cardano_uplc_flat_reader_t* reader, cardano_bigint_t** value);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BIGLUP_LABS_INCLUDE_CARDANO_UPLC_FLAT_FLAT_READER_H */
