/**
 * \file account_balance_interval.h
 *
 * \author angel.castillo
 * \date   Jul 16, 2026
 *
 * Copyright 2026 Biglup Labs
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_ACCOUNT_BALANCE_INTERVAL_H
#define BIGLUP_LABS_INCLUDE_CARDANO_ACCOUNT_BALANCE_INTERVAL_H

/* INCLUDES ******************************************************************/

#include <cardano/cbor/cbor_reader.h>
#include <cardano/cbor/cbor_writer.h>
#include <cardano/error.h>
#include <cardano/export.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Represents a half open interval of lovelace amounts.
 *
 * The interval is encoded on the wire as a two element CBOR array where the first
 * element is the inclusive lower bound and the second element is the exclusive upper
 * bound. Either bound may be encoded as CBOR null to indicate that the interval is
 * unbounded on that side, but at least one bound must be present.
 */
typedef struct cardano_account_balance_interval_t cardano_account_balance_interval_t;

/**
 * \brief Creates and initializes a new instance of an account balance interval.
 *
 * This function allocates and initializes a new instance of \ref cardano_account_balance_interval_t,
 * representing a half open interval of lovelace amounts. A bound is omitted by passing NULL for its
 * argument; at least one bound must be provided.
 *
 * \param[in] inclusive_lower_bound A pointer to the inclusive lower bound in lovelace, or NULL if the
 *                                  interval has no lower bound.
 * \param[in] exclusive_upper_bound A pointer to the exclusive upper bound in lovelace, or NULL if the
 *                                  interval has no upper bound.
 * \param[out] account_balance_interval A pointer to a pointer to a \ref cardano_account_balance_interval_t object. Upon successful
 *                        initialization, this will point to a newly created \ref cardano_account_balance_interval_t
 *                        object. This object represents a "strong reference" to the account_balance_interval,
 *                        fully initialized and ready for use. The caller is responsible for managing
 *                        the lifecycle of this object. Specifically, once the account_balance_interval is no longer
 *                        needed, the caller must release it by calling \ref cardano_account_balance_interval_unref.
 *
 * \return \ref CARDANO_SUCCESS if the account_balance_interval was successfully created, \ref CARDANO_ERROR_INVALID_ARGUMENT
 *         if both bounds are NULL, or an appropriate error code indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_account_balance_interval_t* account_balance_interval = NULL;
 * const uint64_t inclusive_lower_bound = 100;
 * const uint64_t exclusive_upper_bound = 5000;
 *
 * // Attempt to create a new account_balance_interval
 * cardano_error_t result = cardano_account_balance_interval_new(&inclusive_lower_bound, &exclusive_upper_bound, &account_balance_interval);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the account_balance_interval
 *
 *   // Once done, ensure to clean up and release the account_balance_interval
 *   cardano_account_balance_interval_unref(&account_balance_interval);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_account_balance_interval_new(
  const uint64_t*                      inclusive_lower_bound,
  const uint64_t*                      exclusive_upper_bound,
  cardano_account_balance_interval_t** account_balance_interval);

/**
 * \brief Creates an account balance interval from a CBOR reader.
 *
 * This function parses CBOR data using a provided \ref cardano_cbor_reader_t and constructs a \ref cardano_account_balance_interval_t object.
 * It assumes that the CBOR reader is set up correctly and that the CBOR data corresponds to the structure expected for an account balance interval.
 *
 * The interval must be a two element CBOR array. A bound encoded as CBOR null is decoded as an
 * absent bound. Decoding fails with \ref CARDANO_ERROR_INVALID_ARGUMENT if both bounds are null.
 *
 * \param[in] reader A pointer to an initialized \ref cardano_cbor_reader_t that is ready to read the CBOR-encoded account balance interval data.
 * \param[out] account_balance_interval A pointer to a pointer of \ref cardano_account_balance_interval_t that will be set to the address
 *                        of the newly created account balance interval object upon successful decoding.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the account_balance_interval was successfully created, or an appropriate error code if an error occurred.
 *
 * \note If the function fails, the last error can be retrieved by calling \ref cardano_cbor_reader_get_last_error with the reader.
 *       The caller is responsible for freeing the created \ref cardano_account_balance_interval_t object by calling
 *       \ref cardano_account_balance_interval_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_new(cbor_data, data_size);
 * cardano_account_balance_interval_t* account_balance_interval = NULL;
 *
 * cardano_error_t result = cardano_account_balance_interval_from_cbor(reader, &account_balance_interval);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the account_balance_interval
 *
 *   // Once done, ensure to clean up and release the account_balance_interval
 *   cardano_account_balance_interval_unref(&account_balance_interval);
 * }
 * else
 * {
 *   const char* error = cardano_cbor_reader_get_last_error(reader);
 *   printf("Failed to decode account_balance_interval: %s\n", error);
 * }
 *
 * cardano_cbor_reader_unref(&reader); // Cleanup the CBOR reader
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_account_balance_interval_from_cbor(cardano_cbor_reader_t* reader, cardano_account_balance_interval_t** account_balance_interval);

/**
 * \brief Serializes an account balance interval into CBOR format using a CBOR writer.
 *
 * This function serializes the given \ref cardano_account_balance_interval_t object using a \ref cardano_cbor_writer_t.
 * The interval is written as a two element CBOR array; an absent bound is written as CBOR null.
 *
 * \param[in] account_balance_interval A constant pointer to the \ref cardano_account_balance_interval_t object that is to be serialized.
 * \param[out] writer A pointer to a \ref cardano_cbor_writer_t where the CBOR serialized data will be written.
 *                    The writer must already be initialized and ready to accept the data.
 *
 * \return Returns \ref CARDANO_SUCCESS if the serialization is successful. If the \p account_balance_interval or \p writer
 *         is NULL, returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_account_balance_interval_t* account_balance_interval = ...;
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * if (writer)
 * {
 *   cardano_error_t result = cardano_account_balance_interval_to_cbor(account_balance_interval, writer);
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
 * cardano_account_balance_interval_unref(&account_balance_interval);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_account_balance_interval_to_cbor(
  const cardano_account_balance_interval_t* account_balance_interval,
  cardano_cbor_writer_t*                    writer);

/**
 * \brief Retrieves the inclusive lower bound of the interval.
 *
 * This function returns a pointer to the inclusive lower bound of the given account balance
 * interval, expressed in lovelace. The bound is optional; a NULL return value indicates that
 * the interval has no lower bound.
 *
 * \param[in] account_balance_interval A constant pointer to the \ref cardano_account_balance_interval_t object from which
 *                       the bound is to be retrieved.
 *
 * \return A pointer to the inclusive lower bound, or NULL if the interval has no lower bound or if
 *         \p account_balance_interval is NULL. The returned pointer refers to internal storage of the
 *         interval object and must not be modified or freed by the caller. It remains valid for the
 *         lifetime of the interval object.
 *
 * Usage Example:
 * \code{.c}
 * cardano_account_balance_interval_t* account_balance_interval = ...;
 *
 * const uint64_t* inclusive_lower_bound = cardano_account_balance_interval_get_inclusive_lower_bound(account_balance_interval);
 *
 * if (inclusive_lower_bound != NULL)
 * {
 *   printf("Inclusive lower bound: %llu\n", (unsigned long long)*inclusive_lower_bound);
 * }
 * else
 * {
 *   printf("The interval has no lower bound.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT const uint64_t* cardano_account_balance_interval_get_inclusive_lower_bound(
  const cardano_account_balance_interval_t* account_balance_interval);

/**
 * \brief Retrieves the exclusive upper bound of the interval.
 *
 * This function returns a pointer to the exclusive upper bound of the given account balance
 * interval, expressed in lovelace. The bound is optional; a NULL return value indicates that
 * the interval has no upper bound.
 *
 * \param[in] account_balance_interval A constant pointer to the \ref cardano_account_balance_interval_t object from which
 *                       the bound is to be retrieved.
 *
 * \return A pointer to the exclusive upper bound, or NULL if the interval has no upper bound or if
 *         \p account_balance_interval is NULL. The returned pointer refers to internal storage of the
 *         interval object and must not be modified or freed by the caller. It remains valid for the
 *         lifetime of the interval object.
 *
 * Usage Example:
 * \code{.c}
 * cardano_account_balance_interval_t* account_balance_interval = ...;
 *
 * const uint64_t* exclusive_upper_bound = cardano_account_balance_interval_get_exclusive_upper_bound(account_balance_interval);
 *
 * if (exclusive_upper_bound != NULL)
 * {
 *   printf("Exclusive upper bound: %llu\n", (unsigned long long)*exclusive_upper_bound);
 * }
 * else
 * {
 *   printf("The interval has no upper bound.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT const uint64_t* cardano_account_balance_interval_get_exclusive_upper_bound(
  const cardano_account_balance_interval_t* account_balance_interval);

/**
 * \brief Decrements the reference count of an account_balance_interval object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_account_balance_interval_t object
 * by decreasing its reference count. When the reference count reaches zero, the account_balance_interval is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] account_balance_interval A pointer to the pointer of the account_balance_interval object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_account_balance_interval_t* account_balance_interval = NULL;
 * const uint64_t inclusive_lower_bound = 100;
 *
 * cardano_error_t result = cardano_account_balance_interval_new(&inclusive_lower_bound, NULL, &account_balance_interval);
 *
 * // Perform operations with the account_balance_interval...
 *
 * cardano_account_balance_interval_unref(&account_balance_interval);
 * // At this point, account_balance_interval is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_account_balance_interval_unref, the pointer to the \ref cardano_account_balance_interval_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_account_balance_interval_unref(cardano_account_balance_interval_t** account_balance_interval);

/**
 * \brief Increases the reference count of the cardano_account_balance_interval_t object.
 *
 * This function is used to manually increment the reference count of an account_balance_interval
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_account_balance_interval_unref.
 *
 * \param account_balance_interval A pointer to the account_balance_interval object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming account_balance_interval is a previously created account_balance_interval object
 *
 * cardano_account_balance_interval_ref(account_balance_interval);
 *
 * // Now account_balance_interval can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_account_balance_interval_ref there is a corresponding
 * call to \ref cardano_account_balance_interval_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_account_balance_interval_ref(cardano_account_balance_interval_t* account_balance_interval);

/**
 * \brief Retrieves the current reference count of the cardano_account_balance_interval_t object.
 *
 * This function returns the number of active references to an account_balance_interval object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_account_balance_interval_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param account_balance_interval A pointer to the account_balance_interval object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified account_balance_interval object. If the object
 * is properly managed (i.e., every \ref cardano_account_balance_interval_ref call is matched with a
 * \ref cardano_account_balance_interval_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming account_balance_interval is a previously created account_balance_interval object
 *
 * size_t ref_count = cardano_account_balance_interval_refcount(account_balance_interval);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_account_balance_interval_refcount(const cardano_account_balance_interval_t* account_balance_interval);

/**
 * \brief Sets the last error message for a given account_balance_interval object.
 *
 * Records an error message in the account_balance_interval's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] account_balance_interval A pointer to the \ref cardano_account_balance_interval_t instance whose last error message is
 *                       to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the account_balance_interval's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_account_balance_interval_set_last_error(cardano_account_balance_interval_t* account_balance_interval, const char* message);

/**
 * \brief Retrieves the last error message recorded for a specific account_balance_interval.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_account_balance_interval_set_last_error for the given
 * account_balance_interval. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] account_balance_interval A pointer to the \ref cardano_account_balance_interval_t instance whose last error
 *                   message is to be retrieved. If the account_balance_interval is NULL, the function
 *                   returns a generic error message indicating the null account_balance_interval.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified account_balance_interval. If the account_balance_interval is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_account_balance_interval_set_last_error for the same account_balance_interval, or until
 *       the account_balance_interval is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_account_balance_interval_get_last_error(const cardano_account_balance_interval_t* account_balance_interval);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_ACCOUNT_BALANCE_INTERVAL_H
