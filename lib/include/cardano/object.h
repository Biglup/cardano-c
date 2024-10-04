/**
 * \file object.h
 *
 * \author luisd.bianchi
 * \date   Mar 03, 2024
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_OBJECT_H
#define BIGLUP_LABS_INCLUDE_CARDANO_OBJECT_H

/* INCLUDES ******************************************************************/

#include <cardano/error.h>
#include <cardano/export.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @brief A pointer to a function that deallocates a Cardano object.
 *
 * This typedef defines the signature for deallocator functions used within the Cardano library.
 * Deallocator functions are responsible for freeing the resources associated with a Cardano object.
 * The function pointed to by this typedef is expected to handle the deallocation of both the object's
 * specific resources (if any) and the object itself.
 *
 * @param[in] data A pointer to the object to be deallocated. While the parameter is of type `void*` to
 * allow for generic handling of different object types, it is intended that the actual object passed
 * to the deallocator function will be a specific type derived from the base `cardano_object_t` structure.
 * Implementations of the deallocator function should cast `data` to the appropriate specific type
 * as needed to correctly free its resources.
 */
typedef void (*cardano_object_deallocator_t)(void* data);

/**
 * \brief Base object type.
 *
 * All objects in the library are derived from this type.
 */
typedef struct cardano_object_t
{
    size_t                       ref_count;
    cardano_object_deallocator_t deallocator;
    char                         last_error[1024];
} cardano_object_t;

/**
 * \brief Decrements the object's reference count.
 *
 * If the reference count reaches zero, the object memory is deallocated.
 *
 * \param[in] object Pointer to the object whose reference count is to be decremented.
 */
CARDANO_EXPORT void cardano_object_unref(cardano_object_t** object);

/**
 * \brief Increments the object's reference count.
 *
 * Ensures that the object remains allocated until the last reference is released.
 *
 * \param[in] object object whose reference count is to be incremented.
 */
CARDANO_EXPORT void cardano_object_ref(cardano_object_t* object);

/**
 * \brief Retrieves the object's current reference count.
 *
 * \warning Does not account for transitive references.
 *
 * \param[in] object Target object.
 * \return Current reference count of the object.
 */
CARDANO_EXPORT size_t cardano_object_refcount(const cardano_object_t* object);

/**
 * \brief Sets the last error message for a given object.
 *
 * This function records an error message in the object's last_error buffer,
 * overwriting any previous message. The message is truncated if it exceeds
 * the buffer size. This function is typically used to store descriptive
 * error information that can be retrieved later with
 * cardano_object_get_last_error.
 *
 * \param[in,out] object A pointer to the cardano_object_t instance whose last error
 *               message is to be set. If the object is NULL, the function
 *               has no effect.
 * \param[in] message A null-terminated string containing the error message to be
 *                recorded. If the message is NULL, the object's last_error
 *                will be set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters due to the fixed size
 *       of the last_error buffer (1024 characters), including the null
 *       terminator. Messages longer than this limit will be truncated.
 */
CARDANO_EXPORT void cardano_object_set_last_error(cardano_object_t* object, const char* message);

/**
 * \brief Retrieves the last error message recorded for a specific object.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_object_set_last_error for the given
 * object. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in,out] object A pointer to the \ref cardano_object_t instance whose last error
 *               message is to be retrieved. If the object is \c NULL, the function
 *               returns a generic error message indicating the null object.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified object. If the object is \c NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_object_set_last_error for the same object, or until
 *       the object is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_object_get_last_error(const cardano_object_t* object);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_OBJECT_H
