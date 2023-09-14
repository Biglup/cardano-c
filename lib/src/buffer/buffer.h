/**
 * \file buffer.h
 *
 * \author angel.castillo
 * \date   Sep 12, 2023
 *
 * \section LICENSE
 *
 * Copyright 2023 Biglup Labs
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

#ifndef CARDANO_BUFFER_H
#define CARDANO_BUFFER_H

/* INCLUDES ******************************************************************/

#include <cardano/cbor/cbor_tag.h>
#include <cardano/error.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

/**
 * \brief A dynamic buffer.
 *
 * This buffer grows automatically as data is added.
 * Its capacity doubles when the size reaches the current capacity.
 */
typedef struct cardano_buffer_t cardano_buffer_t;

/**
 * \brief Creates and initializes an cardano_buffer_t.
 *
 * \param capacity The initial capacity of the buffer.
 * \return A pointer to the created buffer, or NULL if memory allocation fails.
 */
cardano_buffer_t* cardano_buffer_new(size_t capacity);

/**
 * \brief Frees the memory used by an cardano_buffer_t.
 *
 * \param buffer The buffer to free.
 */
void cardano_buffer_free(cardano_buffer_t** buffer);

/**
 * \brief Adds data to the end of the buffer.
 *
 * If the buffer's size reaches its capacity, the buffer's capacity is doubled.
 *
 * \param buffer The buffer to which the data is added.
 * \param data The data to add to the buffer.
 * \param size The size of the data to be written.
 *
 * \return <tt>cardano_error_t</tt> Returns <tt>CARDANO_SUCCESS</tt> on success, member of <tt>cardano_error_t</tt> otherwise.
 */
cardano_error_t cardano_buffer_push(cardano_buffer_t* buffer, byte_t* data, size_t size);

/**
 * \brief Gets a pointer to the buffers data.
 *
 * @param buffer The buffer instance.
 * @return A pointer to the inner byte array. If the given buffer is NULL, this function will return NULL.
 */
byte_t* cardano_buffer_get_data(cardano_buffer_t* buffer);

/**
 * \brief Gets buffer size.
 *
 * @param buffer The buffer instance.
 * @return The buffer size. If the given buffer is NULL, this function will return 0.
 */
size_t cardano_buffer_get_size(cardano_buffer_t* buffer);

/**
 * \brief Gets buffer capacity.
 *
 * @param buffer The buffer instance.
 * @return The buffer capacity. If the given buffer is NULL, this function will return 0.
 */
size_t cardano_buffer_get_capacity(cardano_buffer_t* buffer);

#endif // CARDANO_buffer_H