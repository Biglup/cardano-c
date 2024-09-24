/**
 * \file base58.h
 *
 * \author angel.castillo
 * \date   Apr 01, 2024
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_BASE58_H
#define BIGLUP_LABS_INCLUDE_CARDANO_BASE58_H

/* INCLUDES ******************************************************************/

#include <cardano/error.h>
#include <cardano/export.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Calculates the length of a Base58-encoded string.
 *
 * This function computes the length of a string after it has been encoded using Base58. This is
 * useful for allocating the correct amount of memory for the encoded string.
 *
 * \param[in] data Pointer to the input data to be encoded.
 * \param[in] data_length Length of the input data in bytes.
 *
 * \return The length of the encoded Base58 string, including the null terminator.
 *
 * To calculate the required buffer size for a Base58-encoded string:
 * \code{.c}
 * const byte_t* data = ...; // Your data here
 * size_t data_length = ...; // Length of your data
 *
 * // Calculate the length of the encoded Base58 string
 * size_t encoded_length = cardano_encoding_base58_get_encoded_length(data, data_length);
 *
 * // Allocate memory for the encoded string
 * char* encoded_string = (char*)malloc(encoded_length);
 *
 * if (encoded_string != NULL)
 * {
 *   // Proceed with encoding...
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_encoding_base58_get_encoded_length(const byte_t* data, size_t data_length);

/**
 * \brief Encodes data into a Base58-encoded string.
 *
 * This function encodes binary data into a Base58-encoded string. The encoded
 * string includes a null terminator at the end.
 *
 * \param[in] data Pointer to the input data to be encoded.
 * \param[in] data_length Length of the input data in bytes.
 * \param[out] output Pointer to the output buffer for the encoded string.
 * \param[in] output_length Length of the output buffer in bytes.
 *
 * \return \ref CARDANO_SUCCESS if the encoding was successful, or an appropriate error code otherwise.
 *
 * \note The output buffer should be large enough to hold the encoded string and the null terminator.
 *       Use \c cardano_encoding_base58_get_encoded_length to calculate the exact buffer size required.
 *
 * Example usage:
 * \code{.c}
 * // Data to encode
 * const byte_t data[] = {0x01, 0x02, 0x03, 0x04, 0x05};
 * size_t data_length = sizeof(data);
 *
 * // Calculate required buffer size
 * size_t encoded_length = cardano_encoding_base58_get_encoded_length(data, data_length);
 *
 * // Allocate buffer for encoded string
 * char* encoded_string = (char*)malloc(encoded_length);
 *
 * if (encoded_string == NULL)
 * {
 *   // Handle memory allocation error
 * }
 *
 * // Encode data to Base58
 * cardano_error_t result = cardano_encoding_base58_encode(data, data_length, encoded_string, encoded_length);
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Base58 Encoded: %s\n", encoded_string);
 * }
 * else
 * {
 *   // Handle encoding error
 * }
 *
 * // Free allocated buffer
 * free(encoded_string);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_encoding_base58_encode(const byte_t* data, size_t data_length, char* output, size_t output_length);

/**
 * \brief Calculates the length of the data decoded from a Base58 string.
 *
 * This function computes the length of the data decoded from a Base58-encoded string. It is useful for allocating
 * the correct amount of memory for the decoded data. The function accounts for the variable length nature of Base58
 * encoding and provides the exact length of the decoded binary data
 *
 * \param[in] data Pointer to the Base58-encoded string to be decoded.
 * \param[in] data_length Length of the Base58-encoded string in bytes.
 *
 * \return The length of the decoded data in bytes. This length does not include any potential null terminator as the output
 *         is expected to be binary data. If the input string is invalid or an error occurs during the computation, the function
 *         returns 0.
 *
 * Example usage:
 * \code{.c}
 * // Base58-encoded string
 * const char* encoded_string = "3mJr7AoUCHxNqd";
 * size_t encoded_length = strlen(encoded_string);
 *
 * // Calculate required buffer size for decoded data
 * size_t decoded_length = cardano_encoding_base58_get_decoded_length(encoded_string, encoded_length);
 *
 * // Allocate buffer for decoded data
 * byte_t* decoded_data = (byte_t*)malloc(decoded_length);
 *
 * if (decoded_data == NULL)
 * {
 *   // Handle memory allocation error
 * }
 *
 * // Proceed with decoding (assuming a function cardano_encoding_base58_decode exists)
 * cardano_error_t result = cardano_encoding_base58_decode(encoded_string, encoded_length, decoded_data, decoded_length);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use decoded data
 * }
 * else
 * {
 *   // Handle decoding error
 * }
 *
 * // Free allocated buffer for decoded data
 * free(decoded_data);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_encoding_base58_get_decoded_length(const char* data, size_t data_length);

/**
 * \brief Decodes a Base58-encoded string into binary data.
 *
 * This function decodes a Base58-encoded string, converting it back into its original binary data form.
 *
 * \param[in] input Pointer to the Base58-encoded string that needs to be decoded.
 * \param[in] input_length Length of the Base58-encoded string, in bytes. This should be the exact length of the input string,
 *                         excluding any null terminator if present.
 * \param[out] data Pointer to the buffer where the decoded binary data will be stored. It is the caller's responsibility to ensure
 *                  that the buffer is sufficiently large to hold the decoded data.
 * \param[in] data_length The size of the buffer pointed to by `data`, in bytes. This should be computed in advance to be large enough
 *                        to hold the expected decoded binary data.
 *
 * \return \ref CARDANO_SUCCESS if the decoding was successful. Otherwise, an appropriate error code is returned indicating the failure reason.
 *
 * \note The data buffer should be large enough to hold the decoded binary data. The function `cardano_encoding_base58_get_decoded_length`
 *       can be used to calculate the required buffer size based on the length of the Base58-encoded string.
 *
 * Example usage:
 * \code{.c}
 * // Example Base58-encoded string
 * const char* encoded_string = "3mJr7AoUCHxNqd";
 * size_t encoded_length = strlen(encoded_string);
 *
 * // Calculate required buffer size for decoded data
 * size_t decoded_length = cardano_encoding_base58_get_decoded_length((const byte_t*)encoded_string, encoded_length);
 *
 * // Allocate buffer for decoded data
 * byte_t* decoded_data = (byte_t*)malloc(decoded_length);
 * if (!decoded_data)
 * {
 *   // Handle memory allocation failure
 * }
 *
 * // Perform the decoding
 * cardano_error_t result = cardano_encoding_base58_decode(encoded_string, encoded_length, decoded_data, decoded_length);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the decoded data
 * }
 * else
 * {
 *   // Handle decoding error
 * }
 *
 * // Clean up
 * free(decoded_data);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_encoding_base58_decode(const char* input, size_t input_length, byte_t* data, size_t data_length);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_BASE58_H