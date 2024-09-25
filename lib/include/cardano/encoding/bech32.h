/**
 * \file bech32.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_BECH32_H
#define BIGLUP_LABS_INCLUDE_CARDANO_BECH32_H

/* INCLUDES ******************************************************************/

#include <cardano/error.h>
#include <cardano/export.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Calculates the length of a Bech32-encoded string.
 *
 * This function computes the length of a string after it has been encoded using Bech32 encoding. The length includes the
 * human-readable part (HRP), the data part, and the separator.
 *
 * \param[in] hrp Pointer to the human-readable part (HRP) of the Bech32-encoded data.
 * \param[in] hrp_length The length of the HRP in bytes.
 * \param[in] data Pointer to the binary data that will be encoded in the data part of the Bech32 string.
 * \param[in] data_length The length of the binary data in bytes.
 *
 * \return The length of the Bech32-encoded string including the null terminator. This value can be used to allocate
 *         an appropriately sized buffer for the encoded string.
 *
 * Example usage:
 * \code{.c}
 * // Define the human-readable part and the data to be encoded
 * const char* hrp = "addr";
 * size_t hrp_length = strlen(hrp);
 * byte_t data[] = {0x01, 0x02, 0x03, 0x04};
 * size_t data_length = sizeof(data);
 *
 * // Calculate the required buffer size for the Bech32-encoded string
 * size_t encoded_length = cardano_encoding_bech32_get_encoded_length(hrp, hrp_length, data, data_length);
 *
 * // Allocate the buffer for the Bech32-encoded string
 * char* encoded_string = (char*)malloc(encoded_length);
 * if (!encoded_string)
 * {
 *   // Handle memory allocation failure
 * }
 *
 * // Now you can proceed to encode the data into Bech32 using the allocated buffer
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_encoding_bech32_get_encoded_length(
  const char*   hrp,
  size_t        hrp_length,
  const byte_t* data,
  size_t        data_length);

/**
 * \brief Encodes data into a Bech32-encoded string.
 *
 * This function encodes binary data, along with a human-readable part (HRP), into a Bech32-encoded string.
 *
 * \param[in] hrp Pointer to the human-readable part (HRP) that conveys contextual information about the encoded data.
 * \param[in] hrp_length The length of the HRP in bytes.
 * \param[in] data Pointer to the binary data to be encoded.
 * \param[in] data_length The length of the binary data in bytes.
 * \param[out] output Pointer to the buffer where the Bech32-encoded string will be written. The buffer must be
 *             large enough to hold the encoded string and the null terminator.
 * \param[in] output_length The size of the output buffer in bytes.
 *
 * \return \ref CARDANO_SUCCESS if the encoding was successful, or an appropriate error code otherwise.
 *
 * \note The function requires that the `output` buffer is pre-allocated and its length (`output_length`) is
 *       sufficient to hold the entire Bech32-encoded string including the null terminator. To determine the
 *       necessary buffer size in advance, use the `cardano_encoding_bech32_get_encoded_length` function.
 *
 * Example usage:
 * \code{.c}
 * // Define the human-readable part (HRP) and the data to be encoded
 * const char* hrp = "bc";
 * byte_t data[] = {0x01, 0x02, 0x03, 0x04};
 * size_t data_length = sizeof(data);
 *
 * // Calculate the required buffer size for the Bech32-encoded string
 * size_t encoded_length = cardano_encoding_bech32_get_encoded_length(hrp, strlen(hrp), data, data_length);
 *
 * // Allocate the buffer for the Bech32-encoded string
 * char* encoded_string = (char*)malloc(encoded_length);
 * if (!encoded_string)
 * {
 *   // Handle memory allocation failure
 * }
 *
 * // Encode the data into Bech32
 * cardano_error_t result = cardano_encoding_bech32_encode(hrp, strlen(hrp), data, data_length, encoded_string, encoded_length);
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Bech32 Encoded: %s\n", encoded_string);
 * }
 * else
 * {
 *   // Handle encoding failure
 * }
 *
 * // Clean up
 * free(encoded_string);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_encoding_bech32_encode(
  const char*   hrp,
  size_t        hrp_length,
  const byte_t* data,
  size_t        data_length,
  char*         output,
  size_t        output_length);

/**
 * \brief Calculates the length of the data decoded from a Bech32 string.
 *
 * This function computes the length of the data that will be obtained after decoding a Bech32-encoded string.
 * It also provides the length of the human-readable part (HRP) present in the Bech32 string (including the null terminator).
 *
 * \param[in] data Pointer to the Bech32-encoded data from which the lengths are to be calculated.
 * \param[in] data_length The length of the Bech32-encoded data in bytes.
 * \param[out] hrp_length Pointer to the size_t variable where the length of the HRP will be stored.
 *             The value is set to the length of the HRP found in the Bech32-encoded string.
 *
 * \return The length of the decoded data in bytes. This does not include the length of the HRP.
 *
 * \note The function does not decode the data itself; it merely calculates the length of the decoded data
 *       and the HRP based on the Bech32-encoded string. To decode the actual data, use the
 *       `cardano_encoding_bech32_decode` function.
 *
 * Example usage:
 * \code{.c}
 * // Bech32 encoded string
 * const char* encoded_str = "addr_1qw508d6qejxtdg4y5r3zarvary0c5xw7kv8f3t4";
 * size_t encoded_str_length = strlen(encoded_str);
 *
 * // Calculate the lengths of the decoded data and the HRP
 * size_t hrp_length = 0;
 * size_t decoded_data_length = cardano_encoding_bech32_get_decoded_length(encoded_str, encoded_str_length, &hrp_length);
 *
 * // Now you can allocate memory for the decoded data and the HRP accordingly
 * byte_t* decoded_data = (byte_t*)malloc(decoded_data_length);
 * char* hrp = (char*)malloc(hrp_length);
 *
 * if (!decoded_data || !hrp)
 * {
 *   // Handle memory allocation failure
 * }
 *
 * // Proceed to decode the Bech32 string
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_encoding_bech32_get_decoded_length(
  const char* data,
  size_t      data_length,
  size_t*     hrp_length);

/**
 * \brief Decodes a Bech32-encoded string into its original data and human-readable part (HRP).
 *
 * This function takes a Bech32-encoded string as input and decodes it back into the original binary data along with
 * its human-readable part (HRP). The function requires pre-allocated buffers for both the HRP and the decoded data.
 *
 * \param[in] input The Bech32-encoded string to be decoded.
 * \param[in] input_length The length of the Bech32-encoded string.
 * \param[out] hrp Pre-allocated buffer where the decoded human-readable part (HRP) will be stored.
 * \param[in] hrp_length The size of the pre-allocated HRP buffer. It must be large enough to store the HRP including the null terminator.
 * \param[out] data Pre-allocated buffer where the decoded binary data will be stored.
 * \param[in,out] data_length On input, specifies the size of the pre-allocated data buffer. On output, it is updated to reflect the actual length
 *                            of the decoded data.
 *
 * \return \ref CARDANO_SUCCESS if the decoding is successful, otherwise an error code indicating the failure reason.
 *
 * \note The function will modify the `data_length` parameter to indicate the actual length of the decoded data. Ensure that the
 * buffers for `hrp` and `data` are large enough to store the decoded information. Use `cardano_encoding_bech32_get_decoded_length`
 * to calculate the required buffer sizes.
 *
 * Example usage:
 * \code{.c}
 * const char* encoded_str = "addr_1qw508d6qejxtdg4y5r3zarvary0c5xw7kv8f3t4";
 * size_t input_length = strlen(encoded_str);
 * char hrp[10]; // Assume you know the HRP length or have calculated it
 * byte_t data[32]; // Pre-allocated buffer for decoded data
 * size_t data_length = sizeof(data);
 *
 * cardano_error_t result = cardano_encoding_bech32_decode(encoded_str, input_length, hrp, sizeof(hrp), data, &data_length);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Decoded successfully
 *   printf("HRP: %s\n", hrp);
 *   // Use the decoded data as needed
 * }
 * else
 * {
 *   // Handle decoding error
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_encoding_bech32_decode(
  const char* input,
  size_t      input_length,
  char*       hrp,
  size_t      hrp_length,
  byte_t*     data,
  size_t      data_length);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_BECH32_H