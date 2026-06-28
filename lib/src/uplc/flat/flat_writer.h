/**
 * \file flat_writer.h
 *
 * \author angel.castillo
 * \date   Jun 19, 2026
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_UPLC_FLAT_FLAT_WRITER_H
#define BIGLUP_LABS_INCLUDE_CARDANO_UPLC_FLAT_FLAT_WRITER_H

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
 * \brief MSB-first bit writer over the flat serialization used for Plutus scripts.
 *
 * The exact inverse of \ref cardano_uplc_flat_reader_t. Bits are laid down most
 * significant first within each byte. Whole bytes are accumulated in \c buffer
 * as they fill, while \c current and \c used hold the partial trailing byte
 * that has not yet been flushed. The writer owns \c buffer; the caller releases
 * the finished bytes with \ref cardano_uplc_flat_writer_finish.
 */
typedef struct cardano_uplc_flat_writer_t
{
    cardano_buffer_t* buffer;
    uint8_t           current;
    uint8_t           used;
} cardano_uplc_flat_writer_t;

/**
 * \brief Initializes a writer with an empty, freshly allocated byte buffer.
 *
 * \param[out] writer The writer to initialize.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         \p writer is NULL, or \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED if the
 *         backing buffer cannot be created.
 */
cardano_error_t
cardano_uplc_flat_writer_init(cardano_uplc_flat_writer_t* writer);

/**
 * \brief Releases the writer's backing buffer if it still owns one.
 *
 * Idempotent: a writer whose buffer was already taken by
 * \ref cardano_uplc_flat_writer_finish is left unchanged. After this call the
 * writer must not be used again without re-initialization.
 *
 * \param[in,out] writer The writer to dispose. A NULL \p writer is a no-op.
 */
void
cardano_uplc_flat_writer_dispose(cardano_uplc_flat_writer_t* writer);

/**
 * \brief Writes one bit, most significant bit of the current byte first.
 *
 * \param[in,out] writer The writer to append the bit to.
 * \param[in] value The bit; any non-zero value writes a one.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         \p writer is NULL, or \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED if a
 *         completed byte cannot be appended.
 */
cardano_error_t
cardano_uplc_flat_writer_bit(cardano_uplc_flat_writer_t* writer, uint8_t value);

/**
 * \brief Writes the low \p count bits (0..8) of \p value, most significant first.
 *
 * The inverse of \ref cardano_uplc_flat_reader_bits8. Bit <tt>count - 1</tt> of
 * \p value is written first and bit 0 last. A \p count of 0 writes nothing.
 *
 * \param[in,out] writer The writer to append to.
 * \param[in] value The bits to write, taken from the low \p count positions.
 * \param[in] count The number of bits to write; must be at most 8.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         \p writer is NULL, \ref CARDANO_ERROR_INVALID_ARGUMENT if \p count
 *         exceeds 8, or \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED on a failed
 *         byte append.
 */
cardano_error_t
cardano_uplc_flat_writer_bits8(cardano_uplc_flat_writer_t* writer, uint8_t value, uint8_t count);

/**
 * \brief Writes a natural number as 7-bit little-endian continuation groups.
 *
 * The inverse of \ref cardano_uplc_flat_reader_word. The low 7 bits are emitted
 * first, each group carrying a high continuation bit while more groups remain and
 * a clear high bit on the final group.
 *
 * \param[in,out] writer The writer to append to.
 * \param[in] value The natural number to write.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         \p writer is NULL, or \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED on a
 *         failed byte append.
 */
cardano_error_t
cardano_uplc_flat_writer_word(cardano_uplc_flat_writer_t* writer, size_t value);

/**
 * \brief Writes a non-negative magnitude as 7-bit little-endian groups.
 *
 * The inverse of \ref cardano_uplc_flat_reader_big_word for unbounded magnitudes:
 * the low 7 bits are emitted first, each non-final group carrying a high
 * continuation bit. A zero magnitude writes one zero group.
 *
 * \param[in,out] writer The writer to append to.
 * \param[in] magnitude The non-negative magnitude to write. Must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         any argument is NULL, or \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED if a
 *         byte append or an interior bigint cannot be allocated.
 */
cardano_error_t
cardano_uplc_flat_writer_big_word(cardano_uplc_flat_writer_t* writer, const cardano_bigint_t* magnitude);

/**
 * \brief Writes a signed arbitrary-precision integer (zig-zag then big word).
 *
 * The inverse of \ref cardano_uplc_flat_reader_integer. The zig-zag transform maps
 * a non-negative \c n to <tt>2n</tt> and a negative \c n to <tt>-2n - 1</tt>, and
 * the resulting non-negative magnitude is written with
 * \ref cardano_uplc_flat_writer_big_word.
 *
 * \param[in,out] writer The writer to append to.
 * \param[in] value The signed integer to write. Must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         any argument is NULL, or \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED if a
 *         byte append or an interior bigint cannot be allocated.
 */
cardano_error_t
cardano_uplc_flat_writer_integer(cardano_uplc_flat_writer_t* writer, const cardano_bigint_t* value);

/**
 * \brief Writes filler that pads to a byte boundary with a terminating one bit.
 *
 * The inverse of \ref cardano_uplc_flat_reader_filler. Zero bits are written until
 * one more bit would land on a byte boundary, then a single one bit closes the
 * run. An already byte-aligned cursor emits a whole <tt>0x01</tt> byte.
 *
 * \param[in,out] writer The writer to append to.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         \p writer is NULL, or \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED on a
 *         failed byte append.
 */
cardano_error_t
cardano_uplc_flat_writer_filler(cardano_uplc_flat_writer_t* writer);

/**
 * \brief Writes a flat bytestring: byte-align, then length-prefixed blocks.
 *
 * The inverse of \ref cardano_uplc_flat_reader_bytes. A \ref cardano_uplc_flat_writer_filler
 * byte-aligns the stream, then the data is emitted in blocks each prefixed by a
 * one-byte length in 1..255, and a terminating zero-length block ends the sequence.
 *
 * \param[in,out] writer The writer to append to.
 * \param[in] data The bytes to write. May be NULL only when \p size is 0.
 * \param[in] size The number of bytes in \p data.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         \p writer is NULL or \p data is NULL while \p size is non-zero, or
 *         \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED on a failed byte append.
 */
cardano_error_t
cardano_uplc_flat_writer_bytes(cardano_uplc_flat_writer_t* writer, const byte_t* data, size_t size);

/**
 * \brief Finalizes the stream and hands the accumulated bytes to the caller.
 *
 * Any partial trailing byte is flushed and the backing buffer ownership is
 * transferred to \p out, which the caller releases with \ref cardano_buffer_unref.
 * After this call the writer no longer owns a buffer; dispose is a no-op.
 *
 * \param[in,out] writer The writer to finalize.
 * \param[out] out On success, the assembled bytes; left untouched on failure.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         any argument is NULL, or \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED on a
 *         failed flush of the trailing byte.
 */
cardano_error_t
cardano_uplc_flat_writer_finish(cardano_uplc_flat_writer_t* writer, cardano_buffer_t** out);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BIGLUP_LABS_INCLUDE_CARDANO_UPLC_FLAT_FLAT_WRITER_H */
