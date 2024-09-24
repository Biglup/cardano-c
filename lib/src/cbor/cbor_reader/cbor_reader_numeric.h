/**
 * \file cbor_reader_numeric.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_CBOR_READER_INTERNAL_NUMERIC_H
#define BIGLUP_LABS_INCLUDE_CARDANO_CBOR_READER_INTERNAL_NUMERIC_H

/* INCLUDES ******************************************************************/

#include <cardano/buffer.h>
#include <cardano/cbor/cbor_reader.h>
#include <cardano/error.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

/**
 * \brief Decodes an unsigned integer from a buffer based on the provided CBOR header.
 *
 * This function is designed to decode an unsigned integer encoded in CBOR format from a given buffer.
 * The CBOR encoding for integers uses a header byte to indicate the size and format of the integer that follows.
 * This function interprets the header byte and reads the subsequent bytes from the buffer to reconstruct the
 * unsigned integer value.
 *
 * \param[in] buffer A pointer to the \ref cardano_buffer_t instance containing the CBOR encoded data.
 * \param[in] header The CBOR header byte that precedes the encoded integer. This byte contains information
 *                   about the integer's size and whether it is unsigned.
 * \param[out] unsigned_int A pointer to a uint64_t variable where the decoded unsigned integer value will be stored.
 * \param[out] bytes_read A pointer to a size_t variable where the number of bytes read from the buffer will be stored.
 *                        This includes the header byte and any subsequent bytes that form the integer value.
 *
 * \return A \ref cardano_error_t indicating the outcome of the operation. \ref CARDANO_SUCCESS is returned if the unsigned
 * integer is successfully decoded. If an error occurs, such as if the buffer does not contain enough data to represent
 * the integer or if the header byte is invalid, an appropriate error code will be returned to indicate the specific
 * reason for failure.
 */
cardano_error_t
_cbor_reader_decode_unsigned_integer(cardano_buffer_t* buffer, byte_t header, uint64_t* unsigned_int, size_t* bytes_read);

/**
 * \brief Reads a double-precision floating point number from the CBOR stream.
 *
 * This function reads the next data item from the CBOR stream associated with the reader
 * and interprets it as a double-precision floating point number (major type 7 with additional
 * information indicating a double float). It is expected that the current position in the stream
 * points to the start of a floating point data item. The function decodes the value and stores it
 * in the provided double variable.
 *
 * \param[in] reader A pointer to the \ref cardano_cbor_reader_t instance that represents the CBOR
 * stream from which the double value is to be read. The reader must have been previously initialized
 * and positioned correctly within the CBOR data stream.
 * \param[out] value A pointer to a double variable where the decoded floating point number will be stored.
 * The value is overwritten with the decoded number if the operation succeeds.
 *
 * \return A \ref cardano_error_t indicating the result of the read operation. \ref CARDANO_SUCCESS is
 * returned if a double-precision floating point number is successfully read from the stream and decoded.
 * If the operation fails due to reasons such as incorrect stream positioning, unexpected data format,
 * or end of stream before completing the read, an appropriate error code will be returned to indicate
 * the failure reason.
 */
cardano_error_t
_cbor_reader_read_double(cardano_cbor_reader_t* reader, double* value);

/**
 * \brief Reads a signed integer from the CBOR stream.
 *
 * This function reads the next data item from the CBOR stream associated with the reader
 * and interprets it as a signed integer (major types 0 for positive integers and 1 for negative integers).
 * It decodes the value and stores it in the provided int64_t variable. The function handles both
 * positive and negative integers, adjusting the decoded value appropriately for negative integers.
 *
 * \param[in] reader A pointer to the \ref cardano_cbor_reader_t instance that represents the CBOR
 *                   stream from which the signed integer is to be read.
 * \param[out] value A pointer to an int64_t variable where the decoded signed integer will be stored.
 *
 * \return A \ref cardano_error_t indicating the result of the read operation. \ref CARDANO_SUCCESS is
 * returned if a signed integer is successfully read from the stream and decoded. If the operation fails
 * due to reasons such as incorrect stream positioning, unexpected data format, or end of stream before
 * completing the read, an appropriate error code will be returned to indicate the failure reason.
 */
cardano_error_t
_cbor_reader_read_int(cardano_cbor_reader_t* reader, int64_t* value);

/**
 * \brief Reads an unsigned integer from the CBOR stream.
 *
 * This function reads the next data item from the CBOR stream associated with the reader
 * and interprets it as an unsigned integer (major type 0). It decodes the value and stores
 * it in the provided uint64_t variable. This function is intended for reading positive integers
 * encoded in the CBOR format.
 *
 * \param[in] reader A pointer to the \ref cardano_cbor_reader_t instance that represents the CBOR
 *                   stream from which the unsigned integer is to be read.
 * \param[out] value A pointer to a uint64_t variable where the decoded unsigned integer will be stored.
 *
 * \return A \ref cardano_error_t indicating the result of the read operation. \ref CARDANO_SUCCESS is
 * returned if an unsigned integer is successfully read from the stream and decoded. If the operation fails
 * due to reasons such as incorrect stream positioning, unexpected data format, or end of stream before
 * completing the read, an appropriate error code will be returned to indicate the failure reason.
 */
cardano_error_t
_cbor_reader_read_uint(cardano_cbor_reader_t* reader, uint64_t* value);

/**
 * \brief Decodes and reads a big integer (bignum) from CBOR format.
 *
 * This function reads and decodes a bignum from a provided CBOR reader, following the
 * encoding format specified in RFC 7049, section 2.4.2. Bignums are used to represent
 * integers that are too large to be represented directly in the available integer types
 * of CBOR. The function interprets the appropriate tag (2 for unsigned bignum) and decodes
 * the integer value, ensuring its correct representation as a bignum in the resulting bigint object.
 *
 * \param[in] reader The \ref cardano_cbor_reader_t instance from which the bignum will be read.
 * \param[out] bigint A pointer to a pointer that will be set to the address of the newly created \ref cardano_bigint_t object
 *                    representing the decoded big integer value. The caller is responsible for managing the memory of this object.
 *
 * \return Returns \ref CARDANO_SUCCESS if the bignum value was successfully decoded and read from the
 *         CBOR stream. If the operation encounters an error, such as invalid parameters or issues with
 *         reading from the stream, an appropriate error code is returned indicating the reason for the failure.
 *         Consult the \ref cardano_error_t documentation for details on possible error codes and their meanings.
 */
cardano_error_t
_cardano_reader_read_bigint(cardano_cbor_reader_t* reader, cardano_bigint_t** bigint);

#endif // BIGLUP_LABS_INCLUDE_CARDANO_CBOR_READER_INTERNAL_NUMERIC_H
