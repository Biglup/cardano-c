/**
 * \file cbor_initial_byte.h
 *
 * \author angel.castillo
 * \date   Sep 29, 2023
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

#ifndef CARDANO_CBOR_INITIAL_BYTE_H
#define CARDANO_CBOR_INITIAL_BYTE_H

/* INCLUDES ******************************************************************/

#include "cbor_additional_info.h"
#include <cardano/cbor/cbor_major_type.h>
#include <cardano/error.h>
#include <cardano/export.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * Represents a CBOR initial byte.
 * **/
typedef struct cardano_cbor_initial_byte_t cardano_cbor_initial_byte_t;

/**
 * \brief Creates a new CBOR initial byte.
 *
 * \param major_type[in]        Major type of the CBOR initial byte.
 * \param additional_info[in]   Additional info of the CBOR initial byte.
 *
 * \return The newly created CBOR initial byte or NULL on memory allocation failure.
 * The caller assumes ownership of the returned CBOR initial byte and is responsible for its lifecycle.
 * It must be dereferenced when no longer in use.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_cbor_initial_byte_t* cardano_cbor_initial_byte_new(cbor_major_type_t major_type, cbor_additional_info_t additional_info);

/**
 * Creates a CborInitialByte class from a packed initialByte.
 *
 * \param cardano_cbor_initial_byte_t The packed initialByte.
 * \return The newly created CborInitialByte or NULL on memory allocation failure.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_cbor_initial_byte_t* cardano_cbor_initial_byte_from(byte_t initial_byte);

/**
 * Gets the packed initial byte.
 *
 * \param cardano_cbor_initial_byte_t The CborInitialByte.
 * \param packed The packed initial byte.
 *
 * \return <tt>cardano_error_t</tt> <tt>CARDANO_SUCCESS</tt> on success; otherwise, an error from <tt>cardano_error_t</tt>.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_cbor_initial_byte_get_packed(cardano_cbor_initial_byte_t* initial_byte, byte_t* packed);

/**
 * Gets the major type of the initial byte.
 *
 * \param cardano_cbor_initial_byte_t The CborInitialByte.
 * \param major_type The major type of the initial byte.
 *
 * \return <tt>cardano_error_t</tt> <tt>CARDANO_SUCCESS</tt> on success; otherwise, an error from <tt>cardano_error_t</tt>.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_cbor_initial_byte_get_major_type(cardano_cbor_initial_byte_t* initial_byte, cbor_major_type_t* major_type);

/**
 * Gets the additional info of the initial byte.
 *
 * \param cardano_cbor_initial_byte_t The CborInitialByte.
 * \param additional_info The additional info of the initial byte.
 *
 * \return <tt>cardano_error_t</tt> <tt>CARDANO_SUCCESS</tt> on success; otherwise, an error from <tt>cardano_error_t</tt>.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_cbor_initial_byte_get_additional_info(cardano_cbor_initial_byte_t* initial_byte, cbor_additional_info_t* additional_info);

/**
 * \brief Decrements the initial_byte's reference count.
 *
 * If the reference count reaches zero, the cardano_cbor_initial_byte_t memory is deallocated.
 *
 * \param initial_byte[in] Pointer to the cardano_cbor_initial_byte_t whose reference count is to be decremented.
 */
CARDANO_EXPORT void cardano_cbor_initial_byte_unref(cardano_cbor_initial_byte_t** initial_byte);

/**
 * \brief Increments the initial_byte's reference count.
 *
 * Ensures that the cardano_cbor_initial_byte_t remains allocated until the last reference is released.
 *
 * \param initial_byte[in] cardano_cbor_initial_byte_t whose reference count is to be incremented.
 */
CARDANO_EXPORT void cardano_cbor_initial_byte_ref(cardano_cbor_initial_byte_t* initial_byte);

/**
 * \brief Retrieves the initial_byte's current reference count.
 *
 * \warning Does not account for transitive references.
 *
 * \param initial_byte[in] Target initial_byte.
 * \return Current reference count of the initial_byte.
 */
CARDANO_EXPORT size_t cardano_cbor_initial_byte_refcount(const cardano_cbor_initial_byte_t* initial_byte);

/**
 * \brief Moves a initial_byte, decrementing its reference count without deallocating.
 *
 * Useful for transferring cardano_cbor_initial_byte_t ownership to functions that will increase the reference count.
 *
 * \warning Memory will leak if the reference count isn't properly managed after a move.
 *
 * \param initial_byte[in] cardano_cbor_initial_byte_t to be moved.
 * \return The cardano_cbor_initial_byte_t with its reference count decremented.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_cbor_initial_byte_t* cardano_cbor_initial_byte_move(cardano_cbor_initial_byte_t* initial_byte);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // CARDANO_CBOR_INITIAL_BYTE_H
