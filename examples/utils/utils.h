/**
 * \file utils.h
 *
 * \author luisd.bianchi
 * \date   Sep 30, 2024
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_UTILS_H
#define BIGLUP_LABS_INCLUDE_CARDANO_UTILS_H

/* INCLUDES ******************************************************************/

#include <cardano/providers/provider.h>
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
cardano_utils_safe_memcpy(void* dest, size_t dest_size, const void* src, size_t src_size);

/**
 * \brief Safe version of strlen that limits the maximum number of characters to inspect.
 *
 * \param str The string to measure.
 * \param max_length Maximum number of characters to inspect.
 *
 * \return Length of the string or \c max_length if the string exceeds max_length.
 */
size_t
cardano_utils_safe_strlen(const char* str, size_t max_length);

/**
 * \brief Sets a NULL terminated message as the inner error message of a provider implementation object.
 *
 * \param provider_impl Provider implementation object.
 * \param message Error message to set.
 */
void
cardano_utils_set_error_message(cardano_provider_impl_t* provider_impl, const char* message);

/**
 * \brief Retrieves the current timestamp in seconds.
 *
 * This function returns the current time as the number of seconds since the Unix epoch (January 1, 1970).
 *
 * \return The current time as a \c uint64_t representing the Unix timestamp in seconds.
 */
uint64_t
cardano_utils_get_time(void);

/**
 * \brief Calculates the elapsed time in seconds since a given start time.
 *
 * This function computes the elapsed time in seconds between a specified start time and the current time.
 *
 * \param[in] start A \c uint64_t representing the start time as a Unix timestamp (in seconds).
 *
 * \return The elapsed time in seconds as a \c uint64_t. If the current time is less than the start time (unlikely under normal circumstances),
 *         the function returns 0.
 *
 * Usage Example:
 * \code{.c}
 * uint64_t start_time = cardano_utils_get_time();
 * // Perform some operations...
 * uint64_t elapsed_time = cardano_utils_get_elapsed_time_since(start_time);
 * printf("Elapsed time: %llu seconds\n", elapsed_time);
 * \endcode
 */
uint64_t
cardano_utils_get_elapsed_time_since(uint64_t start);

/**
 * \brief Suspends the execution of the current thread for a specified number of milliseconds.
 *
 * @param milliseconds Number of milliseconds to sleep.
 */
void
cardano_utils_sleep(uint64_t milliseconds);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_UTILS_H
