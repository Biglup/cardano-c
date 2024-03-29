/**
 * \file allocators_helpers.h
 *
 * \author angel.castillo
 * \date   Mar 13, 2024
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

#ifndef CARDANO_ALLOCATORS_HELPERS
#define CARDANO_ALLOCATORS_HELPERS

/* INCLUDES ******************************************************************/

#include <stddef.h>

/* DECLARATIONS **************************************************************/

/**
 * \brief Resets the counters used by the mock memory allocators.
 *
 * This function resets the internal counters that track how many times the mock memory
 * allocation functions (malloc and realloc variants) have been called.
 */
void
reset_allocators_run_count();

/**
 * \brief A mock version of malloc that simulates an allocation failure on the first call.
 *
 * This function can be used to test the behavior of code when malloc fails immediately,
 * returning NULL without allocating memory.
 *
 * \param size The size of the memory allocation request.
 * \return NULL to simulate allocation failure.
 */
void*
fail_right_away_malloc(size_t size);

/**
 * \brief A mock version of malloc that allows one successful allocation before failing.
 *
 * This function simulates a scenario where malloc succeeds on the first call but fails
 * on subsequent calls.
 *
 * \param size The size of the memory allocation request.
 * \return A pointer to allocated memory on the first call, and NULL on subsequent calls
 *         to simulate allocation failure.
 */
void*
fail_after_one_malloc(size_t size);

/**
 * \brief A mock version of malloc that allows two successful allocation before failing.
 *
 * This function simulates a scenario where malloc succeeds on the first and second call but fails
 * on subsequent calls.
 *
 * \param size The size of the memory allocation request.
 * \return A pointer to allocated memory on the first call, and NULL on subsequent calls
 *         to simulate allocation failure.
 */
void*
fail_after_two_malloc(size_t size);

/**
 * \brief A mock version of realloc that simulates a reallocation failure on the first call.
 *
 * This function is useful for testing how code reacts when realloc fails immediately,
 * returning NULL without reallocating memory.
 *
 * \param ptr Pointer to the memory block to be reallocated.
 * \param size The new size for the memory block.
 * \return NULL to simulate reallocation failure.
 */
void*
fail_right_away_realloc(void* const ptr, size_t size);

#endif // CARDANO_ALLOCATORS_HELPERS
