/**
 * \file buffer.h
 *
 * \author angel.castillo
 * \date   Sep 12, 2023
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_BUFFER_H
#define BIGLUP_LABS_INCLUDE_CARDANO_BUFFER_H

/* INCLUDES ******************************************************************/

#include <cardano/error.h>
#include <cardano/export.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief A dynamic, reference-counted buffer with configurable exponential growth.
 *
 * This structure represents a dynamically sized buffer that uses reference counting for memory management and
 * employs an exponential growth strategy to manage its capacity.
 *
 * The buffer's capacity automatically increases by a default growth factor of 1.5 whenever it becomes full. This
 * approach to resizing is intended to balance between memory usage and the number of reallocations needed, as
 * excessive reallocations can degrade performance.
 *
 * \note The default growth factor is chosen based on a <a href="http://groups.google.com/group/comp.lang.c++.moderated/msg/ba558b4924758e2e">recommendation from Andrew Koenig</a>, suggesting that it should
 * be less than the golden ratio ((1+sqrt(5))/2, approximately 1.6) to ensure optimal performance. Users can
 * configure the growth factor at compilation time by setting the `COLLECTION_GROW_FACTOR` environment variable,
 * allowing for customization of the buffer's resizing behavior according to their needs.
 */
typedef struct cardano_buffer_t cardano_buffer_t;

/**
 * \brief Creates a new dynamic buffer with the specified initial capacity.
 *
 * This function allocates a new dynamic buffer with the given initial capacity. The buffer's capacity will
 * automatically increase in an exponential manner as needed.
 *
 * \param[in] capacity Initial capacity of the buffer. Must be greater than 0. If 0 is provided, this function
 * returns NULL, indicating an error in buffer creation due to invalid capacity.
 *
 * \return A pointer to the newly created \ref cardano_buffer_t instance, or NULL if memory allocation fails.
 * The caller assumes ownership of the returned buffer and is responsible for its lifecycle management.
 * To release the buffer when it is no longer needed, the caller must call \ref cardano_buffer_unref.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_buffer_t* cardano_buffer_new(size_t capacity);

/**
 * \brief Creates a new dynamic buffer and initializes it with a copy of the given data.
 *
 * This function allocates a new dynamic buffer and copies the provided data into it. The size of the buffer is
 * automatically set to accommodate the specified data.
 *
 * \param[in] array A pointer to the data to initialize the buffer with. The data is copied into the new buffer.
 * \param[in] size The size of the data to copy. Specifies how many bytes from \c array should be copied into
 * the newly created buffer.
 *
 * \return A pointer to the newly created \ref cardano_buffer_t instance, or NULL if memory allocation fails.
 * The caller assumes ownership of the returned buffer and is responsible for managing its lifecycle. When
 * the buffer is no longer needed, it must be released by calling \ref cardano_buffer_unref.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_buffer_t* cardano_buffer_new_from(const byte_t* array, size_t size);

/**
 * \brief Concatenates two buffers into a new one, containing the combined data.
 *
 * This function creates a new dynamic buffer that contains the concatenated contents of two given buffers.
 * The new buffer will first include the data from the first buffer (\c lhs), followed by the data from the
 * second buffer (\c rhs). The resulting buffer's capacity is automatically adjusted to accommodate the total
 * combined data size of both source buffers.
 *
 * \param[in] lhs A pointer to the first buffer to concatenate.
 * \param[in] rhs A pointer to the second buffer to concatenate.
 *
 * \return A pointer to a new \ref cardano_buffer_t instance containing the concatenated data of both input buffers,
 * or NULL if memory allocation fails. The caller assumes ownership of the returned buffer and is responsible for its
 * lifecycle management. To properly release the buffer when it is no longer needed, the caller must call
 * \ref cardano_buffer_unref.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_buffer_t* cardano_buffer_concat(const cardano_buffer_t* lhs, const cardano_buffer_t* rhs);

/**
 * \brief Extracts a portion of the buffer between specified indices, creating a new buffer with the slice.
 *
 * This function creates a new dynamic buffer that contains a specific slice of the source buffer, defined by the
 * provided start and end indices. The start index is inclusive, meaning the slice will start from the element
 * at this index, while the end index is exclusive, indicating the slice ends at the element before this index.
 * This operation allows for selective copying of segments from a larger buffer into a new, smaller buffer.
 *
 * \param[in] buffer The source buffer from which the slice is to be extracted.
 * \param[in] start The starting index of the slice, inclusive. Must be less than the size of the source buffer.
 * \param[in] end The ending index of the slice, exclusive. Must be greater than or equal to the start index and
 *                less than or equal to the size of the source buffer.
 *
 * \return A pointer to a new \ref cardano_buffer_t instance containing the specified slice of the source buffer,
 * or NULL if the input is invalid or memory allocation fails. The caller assumes ownership of the returned buffer
 * and is responsible for its lifecycle management. To properly release the buffer when it is no longer needed, the
 * caller must call \ref cardano_buffer_unref.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_buffer_t* cardano_buffer_slice(const cardano_buffer_t* buffer, size_t start, size_t end);

/**
 * \brief Creates a new buffer by decoding a given hex string.
 *
 * This function allocates a new dynamic buffer and initializes its content by decoding the provided hex string.
 * The hex string must represent binary data in hexadecimal format. Each pair of hex digits (two characters) is
 * converted into a single byte, and the resulting bytes are stored in the new buffer. The size parameter
 * indicates the number of characters in the hex string and should be even to ensure a complete byte representation.
 *
 * \param[in] hex_string A pointer to the hex string to be decoded.
 * \param[in] size The number of characters in the hex string, excluding the null terminator.
 *
 * \return A pointer to a new \ref cardano_buffer_t instance containing the decoded data from the hex string,
 * or NULL if memory allocation fails or the hex string format is invalid. The caller assumes ownership of the
 * returned buffer and is responsible for its lifecycle management. To properly release the buffer when it is no
 * longer needed, the caller must call \ref cardano_buffer_unref.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_buffer_t* cardano_buffer_from_hex(const char* hex_string, size_t size);

/**
 * \brief Calculates the size needed for a buffer's hexadecimal string representation,
 * including the null terminator.
 *
 * This function determines the size of the buffer required to hold the hexadecimal string
 * representation of a buffer's contents, including space for the null terminator. This size
 * can be used to allocate memory for storing the hexadecimal string generated by \ref cardano_buffer_to_hex.
 *
 * \param[in] buffer The source buffer for which the hexadecimal string size is being calculated.
 *
 * \return The size in bytes required to store the buffer's contents as a hexadecimal string,
 * including the null terminator. If the buffer is NULL or empty, returns 1 for
 * the null terminator alone.
 *
 * Example usage:
 * \code{.c}
 * cardano_buffer_t* buffer = cardano_buffer_new_from_data(someData, dataSize);
 * size_t hex_string_size = cardano_buffer_get_hex_size(buffer);
 * char* hex_string = (char*)malloc(hex_string_size);
 * cardano_error_t result = cardano_buffer_to_hex(buffer, hex_string, hex_string_size);
 *
 * if (hex_string != NULL && result == CARDANO_SUCCESS)
 * {
 *   printf("Hex representation: %s\n", hex_string);
 * }
 *
 * free(hex_string);
 *
 * cardano_buffer_unref(&buffer);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_buffer_get_hex_size(const cardano_buffer_t* buffer);

/**
 * \brief Converts the contents of a buffer into a hexadecimal string representation and writes it
 * to the provided buffer.
 *
 * This function takes a source buffer and generates a hexadecimal string representation of its contents,
 * writing the result to a user-provided buffer. The size of the destination buffer must be sufficient to
 * hold the hexadecimal string and the null terminator. The required size can be determined using the
 * \ref cardano_buffer_get_hex_size function.
 *
 * \param[in] buffer The source buffer from which the hex string will be generated.
 * \param[out] dest The destination buffer where the null-terminated hexadecimal string will be written.
 * \param[in] dest_size The size of the destination buffer. This size must include space for the null terminator.
 *
 * \return On success, returns CARDANO_SUCCESS. On failure, returns an error code indicating the
 * reason for failure, such as insufficient buffer size.
 *
 * Example usage:
 * \code{.c}
 * cardano_buffer_t* buffer = cardano_buffer_new_from_data(someData, dataSize);
 * size_t hex_string_size = cardano_buffer_get_hex_size(buffer);
 * char* hex_string = (char*)malloc(hex_string_size);
 * cardano_error_t result = cardano_buffer_to_hex(buffer, hex_string, hex_string_size);
 *
 * if (hex_string != NULL && result == CARDANO_SUCCESS)
 * {
 *     printf("Hex representation: %s\n", hex_string);
 * }
 *
 * free(hex_string);
 *
 * cardano_buffer_unref(&buffer);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_buffer_to_hex(const cardano_buffer_t* buffer, char* dest, size_t dest_size);

/**
 * \brief Computes the size needed for a null-terminated string representation of the buffer's contents.
 *
 * This function calculates the size of the buffer's content when represented as a null-terminated string.
 * The calculated size includes space for the null terminator. This is useful for allocating an appropriately sized
 * buffer before converting the buffer's binary data into a string with `cardano_buffer_to_str`.
 *
 * \param[in] buffer A pointer to the cardano_buffer_t instance whose string size is being queried.
 *
 * \return The size in bytes required to represent the buffer's content as a null-terminated string, including the null terminator.
 *         Returns 0 if the buffer is NULL or contains no data.
 *
 * Example usage:
 * \code{.c}
 * cardano_buffer_t* myBuffer = // assume this is initialized and contains data
 *
 * size_t strSize = cardano_buffer_get_str_size(myBuffer);
 *
 * // strSize now contains the size needed to store the buffer's data as a string
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_buffer_get_str_size(const cardano_buffer_t* buffer);

/**
 * \brief Converts the content of a buffer to a UTF-8 string.
 *
 * This function takes the binary content of the specified buffer and
 * converts it to a null-terminated string. The caller must ensure
 * that the destination buffer (\c dest) is large enough to hold the resulting
 * string, including the null terminator.
 *
 * The size required for the destination buffer can be determined using
 * \ref cardano_buffer_get_str_size.
 *
 * \param[in] buffer The source buffer containing the binary data to be converted.
 * \param[out] dest The destination buffer where the resulting string will be stored.
 * \param[in] dest_size The size of the destination buffer in bytes.
 *
 * \return \ref CARDANO_SUCCESS if the conversion was successful and the string was
 *         copied to \c dest. If the destination buffer is not large enough to hold the string,
 *         an error code will be returned. Refer to \c cardano_error_t documentation
 *         for details on possible error codes.
 *
 * Example usage:
 * \code{.c}
 * size_t needed_size = cardano_buffer_get_str_size(buffer);
 * char* dest = (char*)malloc(needed_size);
 *
 * if (dest != NULL)
 * {
 *   if (cardano_buffer_to_str(buffer, dest, needed_size) == CARDANO_SUCCESS)
 *   {
 *     printf("Buffer content as string: %s\n", dest);
 *   }
 *   else
 *   {
 *     printf("Failed to convert buffer to string.\n");
 *   }
 *
 *   free(dest);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_buffer_to_str(const cardano_buffer_t* buffer, char* dest, size_t dest_size);

/**
 * \brief Checks if two cardano_buffer_t objects are equal.
 *
 * This function compares the contents of two cardano_buffer_t objects, lhs and rhs, to determine if they are equal.
 * If the contents of both buffers are identical in size and content, the function returns true. Otherwise, it returns false.
 *
 * \param[in] lhs A constant pointer to the first cardano_buffer_t object to be compared.
 * \param[in] rhs A constant pointer to the second cardano_buffer_t object to be compared.
 *
 * \return true if the contents of the buffers are equal, false otherwise.
 */
CARDANO_NODISCARD
CARDANO_EXPORT bool cardano_buffer_equals(const cardano_buffer_t* lhs, const cardano_buffer_t* rhs);

/**
 * \brief Compares two buffer objects lexicographically.
 *
 * This function compares two buffer objects lexicographically and returns an integer indicating
 * their relative order. The comparison is performed byte-by-byte.
 *
 * \param[in] lhs Pointer to the first buffer object.
 * \param[in] rhs Pointer to the second buffer object.
 *
 * \return A negative value if lhs is less than rhs, zero if they are equal, and a positive value if lhs is greater than rhs.
 *
 * Usage Example:
 * \code{.c}
 * cardano_buffer_t* buffer1 = NULL;
 * cardano_buffer_t* buffer2 = NULL;
 *
 * // Assume buffer1 and buffer2 are initialized properly
 *
 * int32_t comparison = cardano_buffer_compare(buffer1, buffer2);
 *
 * if (comparison < 0)
 * {
 *   printf("buffer1 is less than buffer2.\n");
 * }
 * else if (comparison == 0)
 * {
 * {
 *   printf("buffer1 is equal to buffer2.\n");
 * }
 * else
 * {
 *   printf("buffer1 is greater than buffer2.\n");
 * }
 *
 * // Clean up
 * cardano_buffer_unref(&buffer1);
 * cardano_buffer_unref(&buffer2);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT int32_t cardano_buffer_compare(const cardano_buffer_t* lhs, const cardano_buffer_t* rhs);

/**
 * \brief Decrements the buffer's reference count.
 *
 * This function is used to decrement the reference count of a `cardano_buffer_t` instance.
 * When the reference count reaches zero, the memory allocated for the buffer is automatically
 * deallocated. It is important to ensure that `cardano_buffer_unref` is called for each
 * `cardano_buffer_new`, `cardano_buffer_new_from`, `cardano_buffer_concat`, or any other
 * function that returns a new buffer reference to prevent memory leaks.
 *
 * \param[in,out] buffer A double pointer to the buffer whose reference count is to be decremented.
 *                       On invocation, `buffer` should point to a valid `cardano_buffer_t*`.
 *                       After execution, the pointer is set to NULL to prevent further use.
 *
 * Example usage:
 * \code{.c}
 * cardano_buffer_t* my_buffer = cardano_buffer_new(256);
 *
 * // Use the buffer...
 * cardano_buffer_unref(&my_buffer); // Decrement ref count and possibly free the buffer
 * assert(my_buffer == NULL); // myBuffer is now NULL
 *
 * \endcode
 */
CARDANO_EXPORT void cardano_buffer_unref(cardano_buffer_t** buffer);

/**
 * \brief Increments the buffer's reference count.
 *
 * This function is used to increment the reference count of a `cardano_buffer_t` instance,
 * indicating that there is an additional owner of the buffer. This ensures that the buffer
 * remains allocated until the last reference is released. It is essential to match each call
 * to `cardano_buffer_ref` with a corresponding call to `cardano_buffer_unref` to prevent memory
 * leaks by ensuring that the buffer is deallocated when it is no longer in use.
 *
 * \param[in] buffer A pointer to the buffer whose reference count is to be incremented.
 *                   `buffer` must point to a valid `cardano_buffer_t` instance.
 *
 * Example usage:
 * \code{.c}
 * cardano_buffer_t* shared_buffer = cardano_buffer_new(128);
 * cardano_buffer_ref(shared_buffer); // Increment ref count because buffer will be shared
 *
 * // Share the buffer. The receiver should also call cardano_buffer_ref when its done
 * // with the buffer.
 *
 * cardano_buffer_unref(&shared_buffer); // Decrement our own reference when done with the buffer
 *
 * \endcode
 */
CARDANO_EXPORT void cardano_buffer_ref(cardano_buffer_t* buffer);

/**
 * \brief Retrieves the buffer's current reference count.
 *
 * This function returns the current reference count of a `cardano_buffer_t` instance.
 * The reference count indicates how many owners or references to the buffer exist.
 * It is essential for managing the buffer's lifecycle, ensuring it remains allocated
 * until the last reference is released. Note that this function does not account for
 * transitive references, which means it only counts direct references to this buffer
 * instance.
 *
 * \param[in] buffer A pointer to the target buffer. `buffer` must point to a valid
 *                   `cardano_buffer_t` instance.
 * \return The current reference count of the buffer.
 *
 * Example usage:
 * \code{.c}
 * cardano_buffer_t* my_buffer = cardano_buffer_new(256);
 * size_t ref_count = cardano_buffer_refcount(my_buffer);
 *
 * printf("Reference count: %zu\n", ref_count);
 *
 * cardano_buffer_unref(&my_buffer);
 * \endcode
 */
CARDANO_EXPORT size_t cardano_buffer_refcount(const cardano_buffer_t* buffer);

/**
 * \brief Retrieves a direct pointer to the internal data of a buffer.
 *
 * This function provides access to the internal storage of the buffer, allowing for read-only operations on its contents.
 * It is intended for situations where direct access to the data is necessary for performance or interoperability reasons.
 *
 * \warning The returned pointer provides raw, direct access to the buffer's internal data. It must not be used to
 * deallocate it using free or similar memory management functions. The lifecycle of the data pointed to by the returned
 * pointer is managed by the buffer's reference counting mechanism; therefore, the data remains valid as long as the
 * buffer object exists and has not been deallocated.
 *
 * \param[in] buffer The buffer instance from which to retrieve the internal data pointer. The buffer must have been previously
 * created and not yet deallocated.
 *
 * \return Returns a pointer to the internal data of the buffer. If the buffer is empty or if the buffer instance is invalid,
 * returns NULL. The returned pointer provides direct access to the buffer's contents and must be used in accordance with the
 * warning above to avoid unintended consequences.
 */
CARDANO_NODISCARD
CARDANO_EXPORT byte_t* cardano_buffer_get_data(const cardano_buffer_t* buffer);

/**
 * \brief Sets the logical size of the buffer to a specified value.
 *
 * This function updates the internal marker of the buffer to reflect a new logical size,
 * indicating how much of the preallocated memory is considered "used" or contains valid data.
 * It is important to note that this function does not allocate, deallocate, or initialize memory;
 * it merely updates the buffer's state to reflect that a certain portion of its already allocated
 * memory is now in use. This is particularly useful when data is written to the buffer in a manner
 * where the exact amount of data written is known and does not need to be sequentially written or tracked.
 *
 * \warning This function should be used with caution as it directly affects how much of the buffer
 *          is considered valid data.
 *
 * \param buffer A pointer to the `cardano_buffer_t` object whose size is to be set. The buffer
 *               must have been properly initialized and should have enough memory allocated to
 *               accommodate the new size.
 * \param size The new logical size of the buffer. This value must not exceed the buffer's current
 *             allocated memory size.
 *
 * \return Returns `CARDANO_SUCCESS` if the buffer's size was successfully updated. If the function
 *         fails (e.g., due to an invalid buffer pointer, or the specified `size` exceeding the
 *         buffer's allocated memory), a non-zero error code is returned.
 *
 * Example Usage:
 * \code
 * cardano_buffer_t* buffer = cardano_buffer_new(1024);
 *
 * // Write data to the buffer in a non-standard manner, or track written data externally
 *
 * // Artificially set the buffer's size after data has been written or manipulated
 * size_t data_written = 512; // Assume 512 bytes of data are now considered "used"
 * cardano_error_t error = cardano_buffer_set_size(&buffer, data_written);
 *
 * if (error == CARDANO_SUCCESS)
 * {
 *   // The buffer now considers 512 bytes of its allocated memory as "used"
 * }
 *
 * // Clean up
 * cardano_buffer_unref(&buffer);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_buffer_set_size(cardano_buffer_t* buffer, size_t size);

/**
 * \brief Securely wipes the contents of a buffer from memory.
 *
 * The `cardano_buffer_memzero` function is used to securely erase the sensitive data stored in the provided \p buffer.
 * This ensures that the data is no longer recoverable from memory after it has been used.
 *
 * After use, sensitive data should be overwritten. However, traditional approaches like `memset()` or hand-written
 * memory clearing routines can be stripped away by optimizing compilers or linkers, potentially leaving sensitive
 * information exposed.
 *
 * The `cardano_buffer_memzero` function guarantees that the memory is cleared, even in the presence of compiler optimizations.
 * It is especially important to call this function before freeing memory that contains sensitive information, such as
 * cryptographic keys or decrypted data, to prevent the data from remaining in memory.
 *
 * \param[in] buffer A pointer to the buffer whose contents should be securely erased.
 *
 * \see cardano_buffer_unref() for releasing the buffer after calling this function.
 *
 * \example
 * \code
 * // Example usage after processing sensitive data
 * cardano_buffer_t* sensitive_data = ...; // Buffer containing sensitive data
 *
 * // Wipe the buffer contents securely before releasing it
 * cardano_buffer_memzero(sensitive_data);
 * cardano_buffer_unref(sensitive_data);
 * \endcode
 */
CARDANO_EXPORT void cardano_buffer_memzero(cardano_buffer_t* buffer);

/**
 * \brief Retrieves the current size of the buffer, indicating how much data it currently holds.
 *
 * This function provides the actual used space within the buffer, reflecting the amount of data it currently contains.
 * It is useful for understanding the buffer's occupancy and managing data operations, such as reading or processing the buffer's contents.
 *
 * \note The returned size represents the amount of data currently stored in the buffer, which may be less than or equal to
 * the buffer's total capacity. A return value of 0 indicates either that the buffer is empty or that the provided buffer reference
 * is NULL.
 *
 * \param[in] buffer The buffer instance for which to retrieve the used space.
 *
 * \return The size of the used space within the buffer, in bytes. This value ranges from 0 to the buffer's total capacity.
 * Returns 0 if the buffer is NULL, indicating an error condition or uninitialized buffer instance.
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_buffer_get_size(const cardano_buffer_t* buffer);

/**
 * \brief Copies data from a Cardano buffer to a specified destination array.
 *
 * This function safely transfers bytes from a `cardano_buffer_t` object to a provided
 * destination array.
 *
 * \param buffer A pointer to the `cardano_buffer_t` object from which data will be copied.
 *               The buffer must be initialized and contain the data to be copied.
 * \param dest A pointer to the destination array where the data will be copied. This array
 *             must be allocated and have a size large enough to hold the data being copied.
 * \param dest_size The size of the destination array in bytes. This value determines the
 *                  maximum amount of data that can be safely copied to `dest`.
 *
 * \return cardano_error_t Returns `CARDANO_SUCCESS` if the data was successfully copied to the
 *         destination array. If the function fails, a non-zero error code is returned,
 *         indicating issues such as null pointers for `buffer` or `dest`, or an insufficient
 *         `dest_size` to hold the buffer's data. In cases of failure, the content of `dest`
 *         may be partially updated or left unchanged.
 *
 * Example Usage:
 * \code
 * const cardano_buffer_t* source_buffer = ...; // Assume source_buffer is initialized and filled with data
 * byte_t destination_array[1024];
 * const size_t destination_size = sizeof(destination_array);
 *
 * cardano_error_t error = cardano_buffer_copy_bytes(source_buffer, destination_array, destination_size);
 *
 * if (error == CARDANO_SUCCESS)
 * {
 *   // Data has been successfully copied to destination_array
 *   // Proceed with using the data in destination_array as needed
 * }
 * else
 * {
 *   // Handle the error (e.g., log it, attempt recovery, etc.)
 * }
 * \endcode
 *
 * \note The caller is responsible for ensuring that the destination array is properly allocated
 *       and sized to accommodate the data being copied.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_buffer_copy_bytes(const cardano_buffer_t* buffer, byte_t* dest, size_t dest_size);

/**
 * \brief Retrieves the total capacity of the buffer.
 *
 * This function returns the total amount of space allocated for the buffer, indicating how much data
 * it can hold before needing to expand. The capacity of a buffer is managed automatically, growing
 * as needed to accommodate new data added to the buffer. This function provides a way to inspect
 * the current capacity without affecting the buffer's state.
 *
 * \param[in] buffer The buffer instance whose total capacity is being queried. It must have been previously
 * created using one of the buffer creation functions. If the buffer is NULL, indicating an invalid or
 * uninitialized buffer instance, the function returns 0.
 *
 * \return The total capacity of the buffer in bytes, representing the maximum amount of data the buffer
 * can currently hold. If the buffer is NULL, the function returns 0.
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_buffer_get_capacity(const cardano_buffer_t* buffer);

/**
 * \brief Repositions the current position within a buffer to a specified offset.
 *
 * This function adjusts the current pointer position within the given `cardano_buffer_t`
 * object to the specified `position`. It is used to set the read/write position within the
 * buffer, allowing subsequent operations to start from the new location. This can be particularly
 * useful for parsing structured data or for resetting the position to rewrite or reread portions
 * of the buffer.
 *
 * \param buffer A pointer to the `cardano_buffer_t` object whose position is to be adjusted.
 * \param position The new position within the buffer where the current pointer should be set.
 *                 This value must be within the bounds of the buffer's current size.
 *
 * \return cardano_error_t Returns `CARDANO_SUCCESS` if the position within the buffer was
 *         successfully updated. If the function fails, a non-zero error code is returned,
 *         indicating an invalid `buffer`, an out-of-bounds `position`, or other errors
 *         related to manipulating the buffer's position.
 *
 * Example Usage:
 * \code
 * cardano_buffer_t* buffer = ...; // Assume buffer is previously initialized and populated
 * size_t new_position = 10; // Desired position to seek to within the buffer
 *
 * cardano_error_t error = cardano_buffer_seek(buffer, new_position);
 *
 * if (error == CARDANO_SUCCESS)
 * {
 *   // The buffer's current position is now set to `new_position`
 *   // Subsequent operations will start from this position
 * }
 *
 * // Continue with further operations on `buffer`
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_buffer_seek(cardano_buffer_t* buffer, size_t position);

/**
 * \brief Appends data to the end of the buffer, expanding its capacity if necessary.
 *
 * This function allows for the addition of arbitrary data to the end of a dynamically sized buffer. If the
 * buffer does not have enough capacity to accommodate the new data, it is automatically expanded.
 *
 * \param[in] buffer The buffer to which data will be appended. It must be a valid, previously allocated
 * buffer instance. The buffer's capacity will be expanded if there is not enough space for the new data.
 * \param[in] data A pointer to the byte array containing the data to be appended to the buffer. The function
 * copies the specified number of bytes from this array into the buffer.
 * \param[in] size The size of the data in bytes to be appended. This specifies how much of the data
 * pointed to by `data` should be copied into the buffer.
 *
 * \return Returns \ref CARDANO_SUCCESS if the append operation was successful. If the operation encounters an error,
 *        such as invalid parameters or issues with writing to the buffer, an appropriate error code is returned
 *        indicating the reason for the failure. Consult the \ref cardano_error_t documentation for details on possible
 *        error codes and their meanings.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_buffer_write(cardano_buffer_t* buffer, const byte_t* data, size_t size);

/**
 * \brief Reads a specified amount of data from the buffer into a provided output array.
 *
 * This function attempts to copy a given number of bytes from the source buffer into the specified output
 * array. If the buffer does not contain enough data to fulfill the requested read operation, an error is
 * returned.
 *
 * \param[in] buffer The source buffer from which data will be read.
 * \param[out] data An output array where the read data will be copied. The array must have sufficient space
 * to store the number of bytes specified by `bytes_to_read`. It is the caller's responsibility to ensure
 * that the size of `data` is at least as large as `bytes_to_read`.
 * \param[in] bytes_to_read The number of bytes to read from the buffer and copy into the `data` output array.
 * This specifies how much data the function should attempt to transfer from the buffer.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the read operation. \ref CARDANO_SUCCESS
 * is returned if the specified amount of data was successfully read from the buffer and copied into the
 * output array. If the operation fails, for example, because the buffer contains less data than requested,
 * an appropriate error code is returned to indicate the specific reason for failure.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_buffer_read(cardano_buffer_t* buffer, byte_t* data, size_t bytes_to_read);

/**
 * \brief Writes a 16-bit unsigned integer to the specified buffer, encoding the integer in little-endian byte order.
 *
 * This function takes a 16-bit unsigned integer and writes it to the target buffer in little-endian format.
 *
 * \param[in] buffer The buffer into which the 16-bit unsigned integer will be written.
 * \param[in] value The 16-bit unsigned integer value to be written to the buffer. The function will
 * split the integer into bytes according to little-endian order and then store these bytes sequentially in the buffer.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the write operation. If the integer is
 * successfully written to the buffer, \ref CARDANO_SUCCESS is returned. If the operation encounters an
 * error, such as an issue with expanding the buffer's capacity, an error code is returned that identifies
 * the failure reason.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_buffer_write_uint16_le(cardano_buffer_t* buffer, uint16_t value);

/**
 * \brief Writes a 32-bit unsigned integer to the specified buffer, encoding the integer in little-endian byte order.
 *
 * This function takes a 32-bit unsigned integer and writes it to the target buffer in little-endian format.
 *
 * \param[in] buffer The buffer into which the 32-bit unsigned integer will be written.
 * \param[in] value The 32-bit unsigned integer value to be encoded and written to the buffer. The function will
 * split the integer into bytes according to little-endian order and then store these bytes sequentially in the buffer.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the write operation. If the integer value is
 * successfully encoded and stored in the buffer, \ref CARDANO_SUCCESS is returned. In the event of an error, such
 * as failure to expand the buffer's capacity, an error code will be returned that details the specific cause of
 * the failure. Clients are encouraged to check this return value to verify that the write operation was successful
 * and to handle any errors that may have occurred.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_buffer_write_uint32_le(cardano_buffer_t* buffer, uint32_t value);

/**
 * \brief Writes a 64-bit unsigned integer to the specified buffer, encoding the integer in little-endian byte order.
 *
 * This function takes a 64-bit unsigned integer and writes it to the target buffer in little-endian format.
 *
 * \param[in] buffer The buffer into which the 64-bit unsigned integer will be written.
 * \param[in] value The 64-bit unsigned integer value to be encoded and written to the buffer. The function will
 * split the integer into bytes according to little-endian order and then store these bytes sequentially in the buffer.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the write operation. If the integer value is
 * successfully encoded and stored in the buffer, \ref CARDANO_SUCCESS is returned. In the event of an error, such
 * as failure to expand the buffer's capacity, an error code will be returned that details the specific cause of
 * the failure. Clients are encouraged to check this return value to verify that the write operation was successful
 * and to handle any errors that may have occurred.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_buffer_write_uint64_le(cardano_buffer_t* buffer, uint64_t value);

/**
 * \brief Writes a 16-bit signed integer to the specified buffer, encoding the integer in little-endian byte order.
 *
 * This function takes a 16-bit signed integer and writes it to the target buffer in little-endian format.
 *
 * \param[in] buffer The buffer into which the 16-bit signed integer will be written.
 * \param[in] value The 16-bit signed integer value to be written to the buffer. The function will
 * split the integer into bytes according to little-endian order and then store these bytes sequentially in the buffer.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the write operation. If the integer is
 * successfully written to the buffer, \ref CARDANO_SUCCESS is returned. If the operation encounters an
 * error, such as an issue with expanding the buffer's capacity, an error code is returned that identifies
 * the failure reason.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_buffer_write_int16_le(cardano_buffer_t* buffer, int16_t value);

/**
 * \brief Writes a 32-bit signed integer to the specified buffer, encoding the integer in little-endian byte order.
 *
 * This function takes a 32-bit signed integer and writes it to the target buffer in little-endian format.
 *
 * \param[in] buffer The buffer into which the 32-bit signed integer will be written.
 * \param[in] value The 32-bit signed integer value to be written to the buffer. The function will
 * split the integer into bytes according to little-endian order and then store these bytes sequentially in the buffer.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the write operation. If the integer is
 * successfully written to the buffer, \ref CARDANO_SUCCESS is returned. If the operation encounters an
 * error, such as an issue with expanding the buffer's capacity, an error code is returned that identifies
 * the failure reason.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_buffer_write_int32_le(cardano_buffer_t* buffer, int32_t value);

/**
 * \brief Writes a 64-bit signed integer to the specified buffer, encoding the integer in little-endian byte order.
 *
 * This function takes a 64-bit signed integer and writes it to the target buffer in little-endian format.
 *
 * \param[in] buffer The buffer into which the 64-bit signed integer will be written.
 * \param[in] value The 64-bit signed integer value to be written to the buffer. The function will
 * split the integer into bytes according to little-endian order and then store these bytes sequentially in the buffer.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the write operation. If the integer is
 * successfully written to the buffer, \ref CARDANO_SUCCESS is returned. If the operation encounters an
 * error, such as an issue with expanding the buffer's capacity, an error code is returned that identifies
 * the failure reason.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_buffer_write_int64_le(cardano_buffer_t* buffer, int64_t value);

/**
 * \brief Writes a floating-point number to the specified buffer, encoding the number in little-endian byte order.
 *
 * This function takes a floating-point number (float) and writes it to the target buffer in little-endian format.
 *
 * \param[in] buffer The buffer into which the floating-point number will be written.
 * \param[in] value The floating-point number (float) to be written to the buffer. The function will
 * split the float into bytes according to little-endian order and then store these bytes sequentially in
 * the buffer.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the write operation. If the float is
 * successfully written to the buffer, \ref CARDANO_SUCCESS is returned. If the operation encounters an
 * error, such as an issue with expanding the buffer's capacity, an error code is returned that identifies
 * the failure reason.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_buffer_write_float_le(cardano_buffer_t* buffer, float value);

/**
 * \brief Writes a double number to the specified buffer, encoding the number in little-endian byte order.
 *
 * This function takes a double number and writes it to the target buffer in little-endian format.
 *
 * \param[in] buffer The buffer into which the double number will be written.
 * \param[in] value The double number to be written to the buffer. The function will
 * split the double into bytes according to little-endian order and then store these bytes sequentially in
 * the buffer.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the write operation. If the double is
 * successfully written to the buffer, \ref CARDANO_SUCCESS is returned. If the operation encounters an
 * error, such as an issue with expanding the buffer's capacity, an error code is returned that identifies
 * the failure reason.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_buffer_write_double_le(cardano_buffer_t* buffer, double value);

/**
 * \brief Writes a 16-bit unsigned integer to the specified buffer, encoding the integer in big-endian byte order.
 *
 * This function takes a 16-bit unsigned integer and writes it to the target buffer in big-endian format.
 *
 * \param[in] buffer The buffer into which the 16-bit unsigned integer will be written.
 * \param[in] value The 16-bit unsigned integer value to be written to the buffer. The function will
 * split the integer into bytes according to big-endian order and then store these bytes sequentially in the buffer.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the write operation. If the integer is
 * successfully written to the buffer, \ref CARDANO_SUCCESS is returned. If the operation encounters an
 * error, such as an issue with expanding the buffer's capacity, an error code is returned that identifies
 * the failure reason.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_buffer_write_uint16_be(cardano_buffer_t* buffer, uint16_t value);

/**
 * \brief Writes a 32-bit unsigned integer to the specified buffer, encoding the integer in big-endian byte order.
 *
 * This function takes a 32-bit unsigned integer and writes it to the target buffer in big-endian format.
 *
 * \param[in] buffer The buffer into which the 32-bit unsigned integer will be written.
 * \param[in] value The 32-bit unsigned integer value to be written to the buffer. The function will
 * split the integer into bytes according to big-endian order and then store these bytes sequentially in the buffer.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the write operation. If the integer is
 * successfully written to the buffer, \ref CARDANO_SUCCESS is returned. If the operation encounters an
 * error, such as an issue with expanding the buffer's capacity, an error code is returned that identifies
 * the failure reason.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_buffer_write_uint32_be(cardano_buffer_t* buffer, uint32_t value);

/**
 * \brief Writes a 64-bit unsigned integer to the specified buffer, encoding the integer in big-endian byte order.
 *
 * This function takes a 64-bit unsigned integer and writes it to the target buffer in big-endian format.
 *
 * \param[in] buffer The buffer into which the 64-bit unsigned integer will be written.
 * \param[in] value The 64-bit unsigned integer value to be written to the buffer. The function will
 * split the integer into bytes according to big-endian order and then store these bytes sequentially in the buffer.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the write operation. If the integer is
 * successfully written to the buffer, \ref CARDANO_SUCCESS is returned. If the operation encounters an
 * error, such as an issue with expanding the buffer's capacity, an error code is returned that identifies
 * the failure reason.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_buffer_write_uint64_be(cardano_buffer_t* buffer, uint64_t value);

/**
 * \brief Writes a 16-bit signed integer to the specified buffer, encoding the integer in big-endian byte order.
 *
 * This function takes a 16-bit signed integer and writes it to the target buffer in big-endian format.
 *
 * \param[in] buffer The buffer into which the 16-bit signed integer will be written.
 * \param[in] value The 16-bit signed integer value to be written to the buffer. The function will
 * split the integer into bytes according to big-endian order and then store these bytes sequentially in the buffer.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the write operation. If the integer is
 * successfully written to the buffer, \ref CARDANO_SUCCESS is returned. If the operation encounters an
 * error, such as an issue with expanding the buffer's capacity, an error code is returned that identifies
 * the failure reason.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_buffer_write_int16_be(cardano_buffer_t* buffer, int16_t value);

/**
 * \brief Writes a 32-bit signed integer to the specified buffer, encoding the integer in big-endian byte order.
 *
 * This function takes a 32-bit signed integer and writes it to the target buffer in big-endian format.
 *
 * \param[in] buffer The buffer into which the 32-bit signed integer will be written.
 * \param[in] value The 32-bit signed integer value to be written to the buffer. The function will
 * split the integer into bytes according to big-endian order and then store these bytes sequentially in the buffer.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the write operation. If the integer is
 * successfully written to the buffer, \ref CARDANO_SUCCESS is returned. If the operation encounters an
 * error, such as an issue with expanding the buffer's capacity, an error code is returned that identifies
 * the failure reason.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_buffer_write_int32_be(cardano_buffer_t* buffer, int32_t value);

/**
 * \brief Writes a 64-bit signed integer to the specified buffer, encoding the integer in big-endian byte order.
 *
 * This function takes a 64-bit signed integer and writes it to the target buffer in big-endian format.
 *
 * \param[in] buffer The buffer into which the 64-bit signed integer will be written.
 * \param[in] value The 64-bit signed integer value to be written to the buffer. The function will
 * split the integer into bytes according to big-endian order and then store these bytes sequentially in the buffer.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the write operation. If the integer is
 * successfully written to the buffer, \ref CARDANO_SUCCESS is returned. If the operation encounters an
 * error, such as an issue with expanding the buffer's capacity, an error code is returned that identifies
 * the failure reason.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_buffer_write_int64_be(cardano_buffer_t* buffer, int64_t value);

/**
 * \brief Writes a floating-point number to the specified buffer, encoding the number in big-endian byte order.
 *
 * This function takes a floating-point number (float) and writes it to the target buffer in big-endian format.
 *
 * \param[in] buffer The buffer into which the floating-point number will be written.
 * \param[in] value The floating-point number (float) to be written to the buffer. The function will
 * split the float into bytes according to big-endian order and then store these bytes sequentially in
 * the buffer.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the write operation. If the float is
 * successfully written to the buffer, \ref CARDANO_SUCCESS is returned. If the operation encounters an
 * error, such as an issue with expanding the buffer's capacity, an error code is returned that identifies
 * the failure reason.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_buffer_write_float_be(cardano_buffer_t* buffer, float value);

/**
 * \brief Writes a double number to the specified buffer, encoding the number in big-endian byte order.
 *
 * This function takes a double number and writes it to the target buffer in big-endian format.
 *
 * \param[in] buffer The buffer into which the double number will be written.
 * \param[in] value The double number to be written to the buffer. The function will
 * split the double into bytes according to big-endian order and then store these bytes sequentially in
 * the buffer.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the write operation. If the double is
 * successfully written to the buffer, \ref CARDANO_SUCCESS is returned. If the operation encounters an
 * error, such as an issue with expanding the buffer's capacity, an error code is returned that identifies
 * the failure reason.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_buffer_write_double_be(cardano_buffer_t* buffer, double value);

/**
 * \brief Reads a 16-bit unsigned integer from the specified buffer, decoding it from little-endian byte order.
 *
 * This function extracts a 16-bit unsigned integer from the buffer, assuming the number is stored in little-endian format.
 *
 * \param[in] buffer The buffer from which the 16-bit unsigned integer will be read. The buffer's current read position
 * determines the starting point for reading the value.
 * \param[out] value A pointer to a 16-bit unsigned integer variable where the decoded value will be stored.
 * The value is converted from little-endian byte order to the native byte order of the executing platform
 * before being stored.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the read operation. If the 16-bit unsigned
 * integer is successfully read and decoded from the buffer, \ref CARDANO_SUCCESS is returned. If the operation
 * encounters an error, such as attempting to read beyond the available data in the buffer, an error code
 * is returned that identifies the specific failure reason.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_buffer_read_uint16_le(cardano_buffer_t* buffer, uint16_t* value);

/**
 * \brief Reads a 32-bit unsigned integer from the specified buffer, decoding it from little-endian byte order.
 *
 * This function extracts a 32-bit unsigned integer from the buffer, assuming the number is stored in little-endian format.
 *
 * \param[in] buffer The buffer from which the 32-bit unsigned integer will be read. The buffer's current read position
 * determines the starting point for reading the value.
 * \param[out] value A pointer to a 32-bit unsigned integer variable where the decoded value will be stored.
 * The value is converted from little-endian byte order to the native byte order of the executing platform
 * before being stored.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the read operation. If the 32-bit unsigned
 * integer is successfully read and decoded from the buffer, \ref CARDANO_SUCCESS is returned. If the operation
 * encounters an error, such as attempting to read beyond the available data in the buffer, an error code
 * is returned that identifies the specific failure reason.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_buffer_read_uint32_le(cardano_buffer_t* buffer, uint32_t* value);

/**
 * \brief Reads a 64-bit unsigned integer from the specified buffer, decoding it from little-endian byte order.
 *
 * This function extracts a 64-bit unsigned integer from the buffer, assuming the number is stored in little-endian format.
 *
 * \param[in] buffer The buffer from which the 64-bit unsigned integer will be read. The buffer's current read position
 * determines the starting point for reading the value.
 * \param[out] value A pointer to a 64-bit unsigned integer variable where the decoded value will be stored.
 * The value is converted from little-endian byte order to the native byte order of the executing platform
 * before being stored.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the read operation. If the 64-bit unsigned
 * integer is successfully read and decoded from the buffer, \ref CARDANO_SUCCESS is returned. If the operation
 * encounters an error, such as attempting to read beyond the available data in the buffer, an error code
 * is returned that identifies the specific failure reason.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_buffer_read_uint64_le(cardano_buffer_t* buffer, uint64_t* value);

/**
 * \brief Reads a 16-bit signed integer from the specified buffer, decoding it from little-endian byte order.
 *
 * This function extracts a 16-bit signed integer from the buffer, assuming the number is stored in little-endian format.
 *
 * \param[in] buffer The buffer from which the 16-bit signed integer will be read. The buffer's current read position
 * determines the starting point for reading the value.
 * \param[out] value A pointer to a 16-bit signed integer variable where the decoded value will be stored.
 * The value is converted from little-endian byte order to the native byte order of the executing platform
 * before being stored.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the read operation. If the 16-bit signed
 * integer is successfully read and decoded from the buffer, \ref CARDANO_SUCCESS is returned. If the operation
 * encounters an error, such as attempting to read beyond the available data in the buffer, an error code
 * is returned that identifies the specific failure reason.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_buffer_read_int16_le(cardano_buffer_t* buffer, int16_t* value);

/**
 * \brief Reads a 32-bit signed integer from the specified buffer, decoding it from little-endian byte order.
 *
 * This function extracts a 32-bit signed integer from the buffer, assuming the number is stored in little-endian format.
 *
 * \param[in] buffer The buffer from which the 32-bit signed integer will be read. The buffer's current read position
 * determines the starting point for reading the value.
 * \param[out] value A pointer to a 32-bit signed integer variable where the decoded value will be stored.
 * The value is converted from little-endian byte order to the native byte order of the executing platform
 * before being stored.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the read operation. If the 32-bit signed
 * integer is successfully read and decoded from the buffer, \ref CARDANO_SUCCESS is returned. If the operation
 * encounters an error, such as attempting to read beyond the available data in the buffer, an error code
 * is returned that identifies the specific failure reason.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_buffer_read_int32_le(cardano_buffer_t* buffer, int32_t* value);

/**
 * \brief Reads a 64-bit signed integer from the specified buffer, decoding it from little-endian byte order.
 *
 * This function extracts a 64-bit signed integer from the buffer, assuming the number is stored in little-endian format.
 *
 * \param[in] buffer The buffer from which the 64-bit signed integer will be read. The buffer's current read position
 * determines the starting point for reading the value.
 * \param[out] value A pointer to a 64-bit signed integer variable where the decoded value will be stored.
 * The value is converted from little-endian byte order to the native byte order of the executing platform
 * before being stored.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the read operation. If the 64-bit signed
 * integer is successfully read and decoded from the buffer, \ref CARDANO_SUCCESS is returned. If the operation
 * encounters an error, such as attempting to read beyond the available data in the buffer, an error code
 * is returned that identifies the specific failure reason.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_buffer_read_int64_le(cardano_buffer_t* buffer, int64_t* value);

/**
 * \brief Reads a floating-point number from the specified buffer, decoding it from little-endian byte order.
 *
 * This function extracts a floating-point number (float) from the buffer, assuming the number is stored in
 * little-endian format.
 *
 * \param[in] buffer The buffer from which the floating-point number will be read. The buffer's current read position
 * determines the starting point for reading the value.
 * \param[out] value A pointer to a floating-point number variable where the decoded value will be stored.
 * The value is converted from little-endian byte order to the native byte order of the executing platform
 * before being stored.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the read operation. If the floating-point
 * number is successfully read and decoded from the buffer, \ref CARDANO_SUCCESS is returned. If the operation
 * encounters an error, such as attempting to read beyond the available data in the buffer, an error code
 * is returned that identifies the specific failure reason.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_buffer_read_float_le(cardano_buffer_t* buffer, float* value);

/**
 * \brief Reads a double number from the specified buffer, decoding it from little-endian byte order.
 *
 * This function extracts a double number from the buffer, assuming the number is stored in
 * little-endian format.
 *
 * \param[in] buffer The buffer from which the double number will be read. The buffer's current read position
 * determines the starting point for reading the value.
 * \param[out] value A pointer to a double number variable where the decoded value will be stored.
 * The value is converted from little-endian byte order to the native byte order of the executing platform
 * before being stored.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the read operation. If the double
 * number is successfully read and decoded from the buffer, \ref CARDANO_SUCCESS is returned. If the operation
 * encounters an error, such as attempting to read beyond the available data in the buffer, an error code
 * is returned that identifies the specific failure reason.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_buffer_read_double_le(cardano_buffer_t* buffer, double* value);

/**
 * \brief Reads a 16-bit unsigned integer from the specified buffer, decoding it from big-endian byte order.
 *
 * This function extracts a 16-bit unsigned integer from the buffer, assuming the number is stored in big-endian format.
 *
 * \param[in] buffer The buffer from which the 16-bit unsigned integer will be read. The buffer's current read position
 * determines the starting point for reading the value.
 * \param[out] value A pointer to a 16-bit unsigned integer variable where the decoded value will be stored.
 * The value is converted from big-endian byte order to the native byte order of the executing platform
 * before being stored.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the read operation. If the 16-bit unsigned
 * integer is successfully read and decoded from the buffer, \ref CARDANO_SUCCESS is returned. If the operation
 * encounters an error, such as attempting to read beyond the available data in the buffer, an error code
 * is returned that identifies the specific failure reason.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_buffer_read_uint16_be(cardano_buffer_t* buffer, uint16_t* value);

/**
 * \brief Reads a 32-bit unsigned integer from the specified buffer, decoding it from big-endian byte order.
 *
 * This function extracts a 32-bit unsigned integer from the buffer, assuming the number is stored in big-endian format.
 *
 * \param[in] buffer The buffer from which the 32-bit unsigned integer will be read. The buffer's current read position
 * determines the starting point for reading the value.
 * \param[out] value A pointer to a 32-bit unsigned integer variable where the decoded value will be stored.
 * The value is converted from big-endian byte order to the native byte order of the executing platform
 * before being stored.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the read operation. If the 32-bit unsigned
 * integer is successfully read and decoded from the buffer, \ref CARDANO_SUCCESS is returned. If the operation
 * encounters an error, such as attempting to read beyond the available data in the buffer, an error code
 * is returned that identifies the specific failure reason.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_buffer_read_uint32_be(cardano_buffer_t* buffer, uint32_t* value);

/**
 * \brief Reads a 64-bit unsigned integer from the specified buffer, decoding it from big-endian byte order.
 *
 * This function extracts a 64-bit unsigned integer from the buffer, assuming the number is stored in big-endian format.
 *
 * \param[in] buffer The buffer from which the 64-bit unsigned integer will be read. The buffer's current read position
 * determines the starting point for reading the value.
 * \param[out] value A pointer to a 64-bit unsigned integer variable where the decoded value will be stored.
 * The value is converted from big-endian byte order to the native byte order of the executing platform
 * before being stored.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the read operation. If the 64-bit unsigned
 * integer is successfully read and decoded from the buffer, \ref CARDANO_SUCCESS is returned. If the operation
 * encounters an error, such as attempting to read beyond the available data in the buffer, an error code
 * is returned that identifies the specific failure reason.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_buffer_read_uint64_be(cardano_buffer_t* buffer, uint64_t* value);

/**
 * \brief Reads a 16-bit signed integer from the specified buffer, decoding it from big-endian byte order.
 *
 * This function extracts a 16-bit signed integer from the buffer, assuming the number is stored in big-endian format.
 *
 * \param[in] buffer The buffer from which the 16-bit signed integer will be read. The buffer's current read position
 * determines the starting point for reading the value.
 * \param[out] value A pointer to a 16-bit signed integer variable where the decoded value will be stored.
 * The value is converted from big-endian byte order to the native byte order of the executing platform
 * before being stored.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the read operation. If the 16-bit signed
 * integer is successfully read and decoded from the buffer, \ref CARDANO_SUCCESS is returned. If the operation
 * encounters an error, such as attempting to read beyond the available data in the buffer, an error code
 * is returned that identifies the specific failure reason.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_buffer_read_int16_be(cardano_buffer_t* buffer, int16_t* value);

/**
 * \brief Reads a 32-bit signed integer from the specified buffer, decoding it from big-endian byte order.
 *
 * This function extracts a 32-bit signed integer from the buffer, assuming the number is stored in big-endian format.
 *
 * \param[in] buffer The buffer from which the 32-bit signed integer will be read. The buffer's current read position
 * determines the starting point for reading the value.
 * \param[out] value A pointer to a 32-bit signed integer variable where the decoded value will be stored.
 * The value is converted from big-endian byte order to the native byte order of the executing platform
 * before being stored.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the read operation. If the 32-bit signed
 * integer is successfully read and decoded from the buffer, \ref CARDANO_SUCCESS is returned. If the operation
 * encounters an error, such as attempting to read beyond the available data in the buffer, an error code
 * is returned that identifies the specific failure reason.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_buffer_read_int32_be(cardano_buffer_t* buffer, int32_t* value);

/**
 * \brief Reads a 64-bit signed integer from the specified buffer, decoding it from big-endian byte order.
 *
 * This function extracts a 64-bit signed integer from the buffer, assuming the number is stored in big-endian format.
 *
 * \param[in] buffer The buffer from which the 64-bit signed integer will be read. The buffer's current read position
 * determines the starting point for reading the value.
 * \param[out] value A pointer to a 64-bit signed integer variable where the decoded value will be stored.
 * The value is converted from big-endian byte order to the native byte order of the executing platform
 * before being stored.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the read operation. If the 64-bit signed
 * integer is successfully read and decoded from the buffer, \ref CARDANO_SUCCESS is returned. If the operation
 * encounters an error, such as attempting to read beyond the available data in the buffer, an error code
 * is returned that identifies the specific failure reason.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_buffer_read_int64_be(cardano_buffer_t* buffer, int64_t* value);

/**
 * \brief Reads a floating-point number from the specified buffer, decoding it from big-endian byte order.
 *
 * This function extracts a floating-point number (float) from the buffer, assuming the number is stored in
 * big-endian format.
 *
 * \param[in] buffer The buffer from which the floating-point number will be read. The buffer's current read position
 * determines the starting point for reading the value.
 * \param[out] value A pointer to a floating-point number variable where the decoded value will be stored.
 * The value is converted from big-endian byte order to the native byte order of the executing platform
 * before being stored.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the read operation. If the floating-point
 * number is successfully read and decoded from the buffer, \ref CARDANO_SUCCESS is returned. If the operation
 * encounters an error, such as attempting to read beyond the available data in the buffer, an error code
 * is returned that identifies the specific failure reason.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_buffer_read_float_be(cardano_buffer_t* buffer, float* value);

/**
 * \brief Reads a double number from the specified buffer, decoding it from big-endian byte order.
 *
 * This function extracts a double number from the buffer, assuming the number is stored in
 * big-endian format.
 *
 * \param[in] buffer The buffer from which the double number will be read. The buffer's current read position
 * determines the starting point for reading the value.
 * \param[out] value A pointer to a double number variable where the decoded value will be stored.
 * The value is converted from big-endian byte order to the native byte order of the executing platform
 * before being stored.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the read operation. If the double
 * number is successfully read and decoded from the buffer, \ref CARDANO_SUCCESS is returned. If the operation
 * encounters an error, such as attempting to read beyond the available data in the buffer, an error code
 * is returned that identifies the specific failure reason.
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_buffer_read_double_be(cardano_buffer_t* buffer, double* value);

/**
 * \brief Sets the last error message for a given buffer object.
 *
 * Records an error message in the buffer's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] buffer A pointer to the \ref cardano_buffer_t instance whose last error message is
 *                   to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the buffer's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_buffer_set_last_error(cardano_buffer_t* buffer, const char* message);

/**
 * \brief Retrieves the last error message recorded for a specific object.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_buffer_set_last_error for the given
 * object. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] buffer A pointer to the \ref cardano_buffer_t instance whose last error
 *                   message is to be retrieved. If the object is NULL, the function
 *                   returns a generic error message indicating the null object.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified object. If the object is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_buffer_set_last_error for the same object, or until
 *       the object is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_buffer_get_last_error(const cardano_buffer_t* buffer);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_BUFFER_H
