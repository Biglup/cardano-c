/**
 * \file ex_unit_prices.h
 *
 * \author angel.castillo
 * \date   Jun 09, 2024
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_EX_UNIT_PRICES_H
#define BIGLUP_LABS_INCLUDE_CARDANO_EX_UNIT_PRICES_H

/* INCLUDES ******************************************************************/

#include <cardano/cbor/cbor_reader.h>
#include <cardano/cbor/cbor_writer.h>
#include <cardano/common/unit_interval.h>
#include <cardano/error.h>
#include <cardano/export.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Specifies the cost (in Lovelace) of these ExUnits. In essence, they set the
 * "price" for the computational resources used by a smart contract.
 */
typedef struct cardano_ex_unit_prices_t cardano_ex_unit_prices_t;

/**
 * \brief Creates and initializes a new instance of the execution unit prices.
 *
 * This function allocates and initializes a new instance of the execution unit prices,
 * representing the cost of computational resources used by a smart contract.
 *
 * \param[in] memory_prices The price for memory consumption as a unit interval.
 * \param[in] steps_prices The price for CPU steps as a unit interval.
 * \param[out] ex_unit_prices On successful initialization, this will point to a newly created
 *            execution unit prices object. This object represents a "strong reference",
 *            meaning that it is fully initialized and ready for use.
 *            The caller is responsible for managing the lifecycle of this object.
 *            Specifically, once the execution unit prices is no longer needed, the caller must release it
 *            by calling \ref cardano_ex_unit_prices_unref.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the execution unit prices was successfully created, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_ex_unit_prices_t* ex_unit_prices = NULL;
 * cardano_unit_interval_t* memory_prices = NULL;
 * cardano_unit_interval_t* steps_prices = NULL;
 * cardano_error_t result = CARDANO_SUCCESS;
 *
 * // Initialize the unit intervals for memory and steps prices
 * result = cardano_unit_interval_from_double(0.003, &memory_prices);
 *
 * if (result != CARDANO_SUCCESS)
 * {
 *   // Handle error
 * }
 *
 * result = cardano_unit_interval_from_double(0.007, &steps_prices);
 *
 * if (result != CARDANO_SUCCESS)
 * {
 *   // Handle error
 * }
 *
 * // Attempt to create a new execution unit prices object
 * result = cardano_ex_unit_prices_new(memory_prices, steps_prices, &ex_unit_prices);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the ex_unit_prices
 *
 *   // Once done, ensure to clean up and release the ex_unit_prices
 *   cardano_ex_unit_prices_unref(&ex_unit_prices);
 * }
 *
 * // Clean up
 * cardano_unit_interval_unref(&memory_prices);
 * cardano_unit_interval_unref(&steps_prices);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_ex_unit_prices_new(
  cardano_unit_interval_t*   memory_prices,
  cardano_unit_interval_t*   steps_prices,
  cardano_ex_unit_prices_t** ex_unit_prices);

/**
 * \brief Creates a \ref cardano_ex_unit_prices_t from a CBOR reader.
 *
 * This function parses CBOR data using a provided \ref cardano_cbor_reader_t and constructs a \ref cardano_ex_unit_prices_t object.
 * It assumes that the CBOR reader is set up correctly and that the CBOR data corresponds to the structure expected for a ex_unit_prices.
 *
 * \param[in] reader A pointer to an initialized \ref cardano_cbor_reader_t that is ready to read the CBOR-encoded data.
 * \param[out] ex_unit_prices A pointer to a pointer of \ref cardano_ex_unit_prices_t that will be set to the address
 *                        of the newly created ex_unit_prices object upon successful decoding.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the object was successfully created, or an appropriate error code if an error occurred.
 *
 * \note If the function fails, the last error can be retrieved by calling \ref cardano_cbor_reader_get_last_error with the reader.
 *       The caller is responsible for freeing the created \ref cardano_ex_unit_prices_t object by calling
 *       \ref cardano_ex_unit_prices_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_new(cbor_data, data_size);
 * cardano_ex_unit_prices_t* ex_unit_prices = NULL;
 *
 * cardano_error_t result = cardano_ex_unit_prices_from_cbor(reader, &ex_unit_prices);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the ex_unit_prices
 *
 *   // Once done, ensure to clean up and release the ex_unit_prices
 *   cardano_ex_unit_prices_unref(&ex_unit_prices);
 * }
 * else
 * {
 *   const char* error = cardano_cbor_reader_get_last_error(reader);
 *   printf("Failed to decode ex_unit_prices: %s\n", error);
 * }
 *
 * cardano_cbor_reader_unref(&reader); // Cleanup the CBOR reader
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_ex_unit_prices_from_cbor(cardano_cbor_reader_t* reader, cardano_ex_unit_prices_t** ex_unit_prices);

/**
 * \brief Serializes protocol version into CBOR format using a CBOR writer.
 *
 * This function serializes the given \ref cardano_ex_unit_prices_t object using a \ref cardano_cbor_writer_t.
 *
 * \param[in] ex_unit_prices A constant pointer to the \ref cardano_ex_unit_prices_t object that is to be serialized.
 * \param[out] writer A pointer to a \ref cardano_cbor_writer_t where the CBOR serialized data will be written.
 *                    The writer must already be initialized and ready to accept the data.
 *
 * \return Returns \ref CARDANO_SUCCESS if the serialization is successful. If the \p ex_unit_prices or \p writer
 *         is NULL, returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_ex_unit_prices_t* ex_unit_prices = ...;
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * if (writer)
 * {
 *   cardano_error_t result = cardano_ex_unit_prices_to_cbor(ex_unit_prices, writer);
 *
 *   if (result == CARDANO_SUCCESS)
 *   {
 *     // Use the writer's buffer containing the serialized data
 *   }
 *   else
 *   {
 *     const char* error_message = cardano_cbor_writer_get_last_error(writer);
 *     printf("Serialization failed: %s\n", error_message);
 *   }
 *
 *   cardano_cbor_writer_unref(&writer);
 * }
 *
 * cardano_ex_unit_prices_unref(&ex_unit_prices);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_ex_unit_prices_to_cbor(
  const cardano_ex_unit_prices_t* ex_unit_prices,
  cardano_cbor_writer_t*          writer);

/**
 * \brief Retrieves the memory price from the execution unit prices.
 *
 * This function returns the memory price from the execution unit prices object.
 *
 * \param[in] ex_unit_prices Pointer to the execution unit prices object.
 * \param[out] memory_prices On successful retrieval, this will point to the memory price
 *                           as a unit interval.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the memory price was successfully retrieved, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_ex_unit_prices_t* ex_unit_prices = ...;
 * cardano_unit_interval_t* memory_prices = NULL;
 *
 * // Assume ex_unit_prices is initialized properly
 * cardano_error_t result = cardano_ex_unit_prices_get_memory_prices(ex_unit_prices, &memory_prices);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the memory_prices
 *   // Clean up
 *   cardano_unit_interval_unref(&memory_prices);
 * }
 *
 * cardano_ex_unit_prices_unref(&ex_unit_prices);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_ex_unit_prices_get_memory_prices(
  cardano_ex_unit_prices_t* ex_unit_prices,
  cardano_unit_interval_t** memory_prices);

/**
 * \brief Retrieves the steps price from the execution unit prices.
 *
 * This function returns the steps price from the execution unit prices object.
 *
 * \param[in] ex_unit_prices Pointer to the execution unit prices object.
 * \param[out] steps_prices On successful retrieval, this will point to the steps price
 *                          as a unit interval.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the steps price was successfully retrieved, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_ex_unit_prices_t* ex_unit_prices = NULL;
 * cardano_unit_interval_t* steps_prices = NULL;
 *
 * // Assume ex_unit_prices is initialized properly
 *
 * cardano_error_t result = cardano_ex_unit_prices_get_steps_prices(ex_unit_prices, &steps_prices);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the steps_prices
 *   // Clean up
 *   cardano_unit_interval_unref(&steps_prices);
 * }
 *
 * cardano_ex_unit_prices_unref(&ex_unit_prices);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_ex_unit_prices_get_steps_prices(
  cardano_ex_unit_prices_t* ex_unit_prices,
  cardano_unit_interval_t** steps_prices);

/**
 * \brief Sets the memory price in the execution unit prices.
 *
 * This function sets the memory price in the execution unit prices object.
 *
 * \param[in] ex_unit_prices Pointer to the execution unit prices object.
 * \param[in] memory_prices Pointer to the unit interval representing the memory price.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the memory price was successfully set, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_ex_unit_prices_t* ex_unit_prices = NULL;
 * cardano_unit_interval_t* memory_prices = NULL;
 *
 * // Assume ex_unit_prices and memory_prices are initialized properly
 *
 * cardano_error_t result = cardano_ex_unit_prices_set_memory_prices(ex_unit_prices, memory_prices);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Memory price was successfully set
 * }
 *
 * // Clean up
 * cardano_ex_unit_prices_unref(&ex_unit_prices);
 * cardano_unit_interval_unref(&memory_prices);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_ex_unit_prices_set_memory_prices(
  cardano_ex_unit_prices_t* ex_unit_prices,
  cardano_unit_interval_t*  memory_prices);

/**
 * \brief Sets the steps price in the execution unit prices.
 *
 * This function sets the steps price in the execution unit prices object.
 *
 * \param[in] ex_unit_prices Pointer to the execution unit prices object.
 * \param[in] steps_prices Pointer to the unit interval representing the steps price.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the steps price was successfully set, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_ex_unit_prices_t* ex_unit_prices = NULL;
 * cardano_unit_interval_t* steps_prices = NULL;
 *
 * // Assume ex_unit_prices and steps_prices are initialized properly
 *
 * cardano_error_t result = cardano_ex_unit_prices_set_steps_prices(ex_unit_prices, steps_prices);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Steps price was successfully set
 * }
 *
 * // Clean up
 * cardano_ex_unit_prices_unref(&ex_unit_prices);
 * cardano_unit_interval_unref(&steps_prices);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_ex_unit_prices_set_steps_prices(
  cardano_ex_unit_prices_t* ex_unit_prices,
  cardano_unit_interval_t*  steps_prices);

/**
 * \brief Decrements the reference count of a cardano_ex_unit_prices_t object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_ex_unit_prices_t object
 * by decreasing its reference count. When the reference count reaches zero, the ex_unit_prices is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] ex_unit_prices A pointer to the pointer of the ex_unit_prices object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_ex_unit_prices_t* ex_unit_prices = cardano_ex_unit_prices_new(major, minor);
 *
 * // Perform operations with the ex_unit_prices...
 *
 * cardano_ex_unit_prices_unref(&ex_unit_prices);
 * // At this point, ex_unit_prices is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_ex_unit_prices_unref, the pointer to the \ref cardano_ex_unit_prices_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_ex_unit_prices_unref(cardano_ex_unit_prices_t** ex_unit_prices);

/**
 * \brief Increases the reference count of the cardano_ex_unit_prices_t object.
 *
 * This function is used to manually increment the reference count of an cardano_ex_unit_prices_t
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_ex_unit_prices_unref.
 *
 * \param ex_unit_prices A pointer to the cardano_ex_unit_prices_t object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming ex_unit_prices is a previously created ex_unit_prices object
 *
 * cardano_ex_unit_prices_ref(ex_unit_prices);
 *
 * // Now ex_unit_prices can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_ex_unit_prices_ref there is a corresponding
 * call to \ref cardano_ex_unit_prices_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_ex_unit_prices_ref(cardano_ex_unit_prices_t* ex_unit_prices);

/**
 * \brief Retrieves the current reference count of the cardano_ex_unit_prices_t object.
 *
 * This function returns the number of active references to an cardano_ex_unit_prices_t object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_ex_unit_prices_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param ex_unit_prices A pointer to the cardano_ex_unit_prices_t object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified cardano_ex_unit_prices_t object. If the object
 * is properly managed (i.e., every \ref cardano_ex_unit_prices_ref call is matched with a
 * \ref cardano_ex_unit_prices_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming ex_unit_prices is a previously created ex_unit_prices object
 *
 * size_t ref_count = cardano_ex_unit_prices_refcount(ex_unit_prices);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_ex_unit_prices_refcount(const cardano_ex_unit_prices_t* ex_unit_prices);

/**
 * \brief Sets the last error message for a given cardano_ex_unit_prices_t object.
 *
 * Records an error message in the ex_unit_prices's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] ex_unit_prices A pointer to the \ref cardano_ex_unit_prices_t instance whose last error message is
 *                       to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the ex_unit_prices's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_ex_unit_prices_set_last_error(
  cardano_ex_unit_prices_t* ex_unit_prices,
  const char*               message);

/**
 * \brief Retrieves the last error message recorded for a specific ex_unit_prices.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_ex_unit_prices_set_last_error for the given
 * ex_unit_prices. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] ex_unit_prices A pointer to the \ref cardano_ex_unit_prices_t instance whose last error
 *                   message is to be retrieved. If the ex_unit_prices is NULL, the function
 *                   returns a generic error message indicating the null ex_unit_prices.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified ex_unit_prices. If the ex_unit_prices is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_ex_unit_prices_set_last_error for the same ex_unit_prices, or until
 *       the ex_unit_prices is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_ex_unit_prices_get_last_error(
  const cardano_ex_unit_prices_t* ex_unit_prices);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_EX_UNIT_PRICES_H