/**
 * \file allocators.h
 *
 * \author luisd.bianchi
 * \date   Mar 11, 2024
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_ALLOCATORS_H
#define BIGLUP_LABS_INCLUDE_CARDANO_ALLOCATORS_H

/* INCLUDES ******************************************************************/

#include <cardano/export.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Function pointer type for custom memory allocation.
 *
 * This typedef defines a custom function signature for memory allocation. Functions matching this
 * signature can be used to replace the standard malloc function.
 *
 * \param size The size in bytes to allocate.
 * \return A pointer to the allocated memory, or NULL if the allocation fails.
 */
typedef void* (*_cardano_malloc_t)(size_t size);

/**
 * \brief Function pointer type for custom memory reallocation.
 *
 * Defines a custom function signature for reallocating memory. Functions with this signature can be used
 * to replace the standard realloc function.
 *
 * \param ptr Pointer to the previously allocated memory block.
 * \param size The new size in bytes for the memory block.
 * \return A pointer to the reallocated memory, or NULL if the reallocation fails.
 */
typedef void* (*_cardano_realloc_t)(void* ptr, size_t size);

/**
 * \brief Function pointer type for custom memory deallocation.
 *
 * Defines a custom function signature for freeing allocated memory. Functions that match this signature
 * can replace the standard free function.
 *
 * \param ptr Pointer to the memory block to be freed.
 */
typedef void (*_cardano_free_t)(void* ptr);

/**
 * \brief Function used for custom memory allocation.
 *
 * User-defined function that allocates memory, following the _cardano_malloc_t signature.
 */
CARDANO_EXPORT void* _cardano_malloc(size_t size);

/**
 * \brief Pointer to a function used for custom memory reallocation.
 *
 * User-defined function for reallocating memory, conforming to the _cardano_realloc_t signature.
 */
CARDANO_EXPORT void* _cardano_realloc(void* ptr, size_t size);

/**
 * \brief Pointer to a function used for custom memory deallocation.
 *
 * User-defined function that frees allocated memory, according to the _cardano_free_t signature.
 */
CARDANO_EXPORT void _cardano_free(void* ptr);

/**
 * \brief Sets the memory management routines to use.
 *
 * By default, libcardano-c will use the standard library `malloc`, `realloc`, and `free`.
 *
 * \warning This function modifies the global state and should therefore be used with caution.
 * Changing the memory handlers while allocated items exist will result in a `free`/`malloc` mismatch.
 * This function is not thread-safe with respect to both itself and all other libcardano-c functions
 * that work with the heap.
 *
 * \note The `realloc` implementation must correctly support `NULL` reallocation
 * (see [realloc documentation](http://en.cppreference.com/w/c/memory/realloc)).
 *
 * \param custom_malloc  Function pointer to the custom malloc implementation.
 * \param custom_realloc Function pointer to the custom realloc implementation.
 * \param custom_free    Function pointer to the custom free implementation.
 */
CARDANO_EXPORT void cardano_set_allocators(_cardano_malloc_t custom_malloc, _cardano_realloc_t custom_realloc, _cardano_free_t custom_free);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_ALLOCATORS_H