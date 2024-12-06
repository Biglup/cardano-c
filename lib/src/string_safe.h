/**
 * \file string_safe.h
 *
 * \author luisd.bianchi
 * \date   Apr 28, 2024
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_STRING_SAFE_H
#define BIGLUP_LABS_INCLUDE_CARDANO_STRING_SAFE_H

/* INCLUDES ******************************************************************/

#include <cardano/error.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Safe version of memcpy that copies up to buffer size into the destination buffer.
 *
 * \param dest Destination buffer.
 * \param dest_size Size of the destination buffer.
 * \param src Source buffer.
 * \param src_size Number of bytes to copy.
 */
void
cardano_safe_memcpy(void* dest, size_t dest_size, const void* src, size_t src_size);

/**
 * \brief Safe version of strlen that limits the maximum number of characters to inspect.
 *
 * \param str The string to measure.
 * \param max_length Maximum number of characters to inspect.
 *
 * \return Length of the string or \c max_length if the string exceeds max_length.
 */
size_t
cardano_safe_strlen(const char* str, size_t max_length);

/**
 * \brief Safely converts a 64-bit integer to a string.
 *
 * \param[in] value The 64-bit integer value to convert.
 * \param[out] buffer The buffer to store the resulting string.
 * \param[in] buffer_size The size of the buffer.
 *
 * \return The number of characters written to the buffer.
 */
size_t
cardano_safe_int64_to_string(int64_t value, char* buffer, size_t buffer_size);

/**
 * \brief Safely converts a string to a 64-bit integer.
 *
 * \param[in] str The string to convert.
 * \param[in] string_size The size of the string.
 * \param[out] value The resulting 64-bit integer value.
 *
 * \return A \ref cardano_error_t indicating the outcome of the operation.
 */
cardano_error_t cardano_safe_string_to_int64(const char* str, size_t string_size, int64_t* value);

/**
 * \brief Safely converts an unsigned 64-bit integer to a string.
 *
 * \param[in] value The unsigned 64-bit integer value to convert.
 * \param[out] buffer The buffer to store the resulting string.
 * \param[in] buffer_size The size of the buffer.
 *
 * \return The number of characters written to the buffer.
 */
size_t
cardano_safe_uint64_to_string(uint64_t value, char* buffer, size_t buffer_size);

/**
 * \brief Safely converts a string to a unsigned 64-bit integer.
 *
 * \param[in] str The string to convert.
 * \param[in] string_size The size of the string.
 * \param[out] value The resulting 64 64-bit integer value.
 *
 * \return A \ref cardano_error_t indicating the outcome of the operation.
 */
cardano_error_t cardano_safe_string_to_uint64(const char* str, size_t string_size, uint64_t* value);

/**
 * \brief Converts a double to a string.
 *
 * \param[in] value The double value to convert.
 * \param[out] buffer The buffer to store the string representation.
 * \param[in] buffer_size The size of the output buffer.
 * \return The number of characters written (excluding null terminator), or 0 on failure.
 */
size_t
cardano_safe_double_to_string(double value, char* buffer, size_t buffer_size);

/**
 * \brief Converts a string to a double value.
 *
 * \param[in] str The input string to convert.
 * \param[in] string_size The size of the input string.
 * \param[out] value The resulting double value.
 * \return A cardano_error_t indicating success or failure.
 */
cardano_error_t
cardano_safe_string_to_double(const char* str, size_t string_size, double* value);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_STRING_SAFE_H
