/**
 * \file unit_interval.h
 *
 * \author angel.castillo
 * \date   Mar 07, 2024
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_UNIT_INTERVAL_H
#define BIGLUP_LABS_INCLUDE_CARDANO_UNIT_INTERVAL_H

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
 * \brief Represents a rational number as a ratio of two integers.
 *
 * Unit intervals are serialized as Rational Numbers (Tag 30).
 * Rational numbers are numbers that can be expressed as a ratio of two integers:
 * a numerator, written as the top part of a fraction, and the denominator,
 * the bottom part. The value of a rational number is the numerator divided by the denominator.
 */
typedef struct cardano_unit_interval_t cardano_unit_interval_t;

/**
 * \brief Creates and initializes a new instance of a unit interval.
 *
 * This function allocates and initializes a new instance of a unit interval,
 * representing a rational number where the numerator and denominator are both
 * unsigned 64-bit integers. Unit intervals are serialized as Rational Numbers (Tag 30).
 * Rational numbers are numbers that can be expressed as a ratio of two integers:
 * a numerator, usually written as the top part of a fraction, and the denominator,
 * the bottom part. The value of a rational number is the numerator divided by the denominator.
 *
 * \param[in] numerator The numerator of the unit interval.
 * \param[in] denominator The denominator of the unit interval.
 * \param[out] unit_interval On successful initialization, this will point to a newly created
 *            unit interval object. This object represents a "strong reference",
 *            meaning that it is fully initialized and ready for use.
 *            The caller is responsible for managing the lifecycle of this object.
 *            Specifically, once the unit interval is no longer needed, the caller must release it
 *            by calling \ref cardano_unit_interval_unref.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the unit interval was successfully created, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_unit_interval_t* unit_interval = NULL;
 * uint64_t numerator = 3;
 * uint64_t denominator = 4;
 *
 * // Attempt to create a new unit interval object
 * cardano_error_t result = cardano_unit_interval_new(numerator, denominator, &unit_interval);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the unit_interval
 *
 *   // Once done, ensure to clean up and release the unit_interval
 *   cardano_unit_interval_unref(&unit_interval);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_unit_interval_new(
  uint64_t                  numerator,
  uint64_t                  denominator,
  cardano_unit_interval_t** unit_interval);

/**
 * \brief Creates and initializes a new instance of a unit interval from a floating-point value.
 *
 * This function creates a new instance of a unit interval from a floating-point value,
 * representing a rational number. The floating-point value is converted into a fraction
 * where the numerator and denominator are both unsigned 64-bit integers.
 *
 * \param[in] value The floating-point value from which to create the unit interval.
 * \param[out] unit_interval On successful initialization, this will point to a newly created
 *            unit interval object. This object represents a "strong reference",
 *            meaning that it is fully initialized and ready for use.
 *            The caller is responsible for managing the lifecycle of this object.
 *            Specifically, once the unit interval is no longer needed, the caller must release it
 *            by calling \ref cardano_unit_interval_unref.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the unit interval was successfully created, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_unit_interval_t* unit_interval = NULL;
 * double value = 0.25; // Example floating-point value
 *
 * // Attempt to create a new unit interval object from the floating-point value
 * cardano_error_t result = cardano_unit_interval_from_double(value, &unit_interval);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the unit_interval
 *
 *   // Once done, ensure to clean up and release the unit_interval
 *   cardano_unit_interval_unref(&unit_interval);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_unit_interval_from_double(double value, cardano_unit_interval_t** unit_interval);

/**
 * \brief Creates a \ref cardano_unit_interval_t from a CBOR reader.
 *
 * This function parses CBOR data using a provided \ref cardano_cbor_reader_t and constructs a \ref cardano_unit_interval_t object.
 * It assumes that the CBOR reader is set up correctly and that the CBOR data corresponds to the structure expected for a unit_interval.
 *
 * \param[in] reader A pointer to an initialized \ref cardano_cbor_reader_t that is ready to read the CBOR-encoded data.
 * \param[out] unit_interval A pointer to a pointer of \ref cardano_unit_interval_t that will be set to the address
 *                        of the newly created unit_interval object upon successful decoding.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the object was successfully created, or an appropriate error code if an error occurred.
 *
 * \note If the function fails, the last error can be retrieved by calling \ref cardano_cbor_reader_get_last_error with the reader.
 *       The caller is responsible for freeing the created \ref cardano_unit_interval_t object by calling
 *       \ref cardano_unit_interval_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_new(cbor_data, data_size);
 * cardano_unit_interval_t* unit_interval = NULL;
 *
 * cardano_error_t result = cardano_unit_interval_from_cbor(reader, &unit_interval);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the unit_interval
 *
 *   // Once done, ensure to clean up and release the unit_interval
 *   cardano_unit_interval_unref(&unit_interval);
 * }
 * else
 * {
 *   const char* error = cardano_cbor_reader_get_last_error(reader);
 *   printf("Failed to decode unit_interval: %s\n", error);
 * }
 *
 * cardano_cbor_reader_unref(&reader); // Cleanup the CBOR reader
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_unit_interval_from_cbor(cardano_cbor_reader_t* reader, cardano_unit_interval_t** unit_interval);

/**
 * \brief Serializes protocol version into CBOR format using a CBOR writer.
 *
 * This function serializes the given \ref cardano_unit_interval_t object using a \ref cardano_cbor_writer_t.
 *
 * \param[in] unit_interval A constant pointer to the \ref cardano_unit_interval_t object that is to be serialized.
 * \param[out] writer A pointer to a \ref cardano_cbor_writer_t where the CBOR serialized data will be written.
 *                    The writer must already be initialized and ready to accept the data.
 *
 * \return Returns \ref CARDANO_SUCCESS if the serialization is successful. If the \p unit_interval or \p writer
 *         is NULL, returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_unit_interval_t* unit_interval = ...;
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * if (writer)
 * {
 *   cardano_error_t result = cardano_unit_interval_to_cbor(unit_interval, writer);
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
 * cardano_unit_interval_unref(&unit_interval);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_unit_interval_to_cbor(
  const cardano_unit_interval_t* unit_interval,
  cardano_cbor_writer_t*         writer);

/**
 * \brief Retrieves the numerator of the Unit Interval.
 *
 * This function returns the numerator of the Unit Interval,
 *
 * \param[in] unit_interval Pointer to the Unit Interval object.
 *
 * \return The numerator of the Unit Interval.
 *
 * Usage Example:
 * \code{.c}
 * cardano_unit_interval_t* unit_interval = NULL;
 * // Assume unit_interval is initialized properly
 *
 * uint64_t numerator_version = cardano_unit_interval_get_numerator(unit_interval);
 * printf("Major Version: %lu\n", numerator_version);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT uint64_t cardano_unit_interval_get_numerator(
  const cardano_unit_interval_t* unit_interval);

/**
 * \brief Sets the numerator of the Unit Interval.
 *
 * This function sets the numerator of the Unit Interval,
 *
 * \param[in] unit_interval Pointer to the Unit Interval object.
 * \param[in] numerator The numerator to set.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the numerator was successfully set, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_unit_interval_t* unit_interval = NULL;
 * // Assume unit_interval is initialized properly
 *
 * cardano_error_t result = cardano_unit_interval_set_numerator(unit_interval, 2);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Major version set successfully.\n");
 * }
 * else
 * {
 *   printf("Failed to set numerator version: %d\n", result);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_unit_interval_set_numerator(
  cardano_unit_interval_t* unit_interval,
  uint64_t                 numerator);

/**
 * \brief Retrieves the denominator of the Unit Interval.
 *
 * This function returns the denominator of the Unit Interval.
 *
 * \param[in] unit_interval Pointer to the Unit Interval object.
 *
 * \return The denominator of the Unit Interval.
 *
 * Usage Example:
 * \code{.c}
 * cardano_unit_interval_t* unit_interval = NULL;
 * // Assume unit_interval is initialized properly
 *
 * uint64_t denominator_version = cardano_unit_interval_get_denominator(unit_interval);
 * printf("Minor Version: %lu\n", denominator_version);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT uint64_t cardano_unit_interval_get_denominator(const cardano_unit_interval_t* unit_interval);

/**
 * \brief Converts a unit interval to a double-precision floating-point value.
 *
 * This function converts a unit interval, representing a rational number, to a
 * double-precision floating-point value. The rational number is converted into
 * a floating-point representation, providing a decimal approximation of the value.
 *
 * \param[in] unit_interval Pointer to the unit interval object to be converted.
 *
 * \return The double-precision floating-point value representing the unit interval.
 *
 * Usage Example:
 * \code{.c}
 * cardano_unit_interval_t* unit_interval = NULL;
 * // Assume unit_interval is initialized properly
 *
 * double result = cardano_unit_interval_to_double(unit_interval);
 * printf("Result: %f\n", result);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT double cardano_unit_interval_to_double(const cardano_unit_interval_t* unit_interval);

/**
 * \brief Sets the denominator of the Unit Interval.
 *
 * This function sets the denominator of the Unit Interval.
 *
 * \param[in] unit_interval Pointer to the Unit Interval object.
 * \param[in] denominator The denominator to set.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the denominator was successfully set, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_unit_interval_t* unit_interval = NULL;
 * // Assume unit_interval is initialized properly
 *
 * cardano_error_t result = cardano_unit_interval_set_denominator(unit_interval, 1);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Minor version set successfully.\n");
 * }
 * else
 * {
 *   printf("Failed to set denominator version: %d\n", result);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_unit_interval_set_denominator(
  cardano_unit_interval_t* unit_interval,
  uint64_t                 denominator);

/**
 * \brief Decrements the reference count of a cardano_unit_interval_t object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_unit_interval_t object
 * by decreasing its reference count. When the reference count reaches zero, the unit_interval is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] unit_interval A pointer to the pointer of the unit_interval object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_unit_interval_t* unit_interval = cardano_unit_interval_new(numerator, denominator);
 *
 * // Perform operations with the unit_interval...
 *
 * cardano_unit_interval_unref(&unit_interval);
 * // At this point, unit_interval is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_unit_interval_unref, the pointer to the \ref cardano_unit_interval_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_unit_interval_unref(cardano_unit_interval_t** unit_interval);

/**
 * \brief Increases the reference count of the cardano_unit_interval_t object.
 *
 * This function is used to manually increment the reference count of an cardano_unit_interval_t
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_unit_interval_unref.
 *
 * \param unit_interval A pointer to the cardano_unit_interval_t object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming unit_interval is a previously created unit_interval object
 *
 * cardano_unit_interval_ref(unit_interval);
 *
 * // Now unit_interval can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_unit_interval_ref there is a corresponding
 * call to \ref cardano_unit_interval_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_unit_interval_ref(cardano_unit_interval_t* unit_interval);

/**
 * \brief Retrieves the current reference count of the cardano_unit_interval_t object.
 *
 * This function returns the number of active references to an cardano_unit_interval_t object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_unit_interval_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param unit_interval A pointer to the cardano_unit_interval_t object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified cardano_unit_interval_t object. If the object
 * is properly managed (i.e., every \ref cardano_unit_interval_ref call is matched with a
 * \ref cardano_unit_interval_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming unit_interval is a previously created unit_interval object
 *
 * size_t ref_count = cardano_unit_interval_refcount(unit_interval);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_unit_interval_refcount(const cardano_unit_interval_t* unit_interval);

/**
 * \brief Sets the last error message for a given cardano_unit_interval_t object.
 *
 * Records an error message in the unit_interval's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] unit_interval A pointer to the \ref cardano_unit_interval_t instance whose last error message is
 *                       to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the unit_interval's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_unit_interval_set_last_error(
  cardano_unit_interval_t* unit_interval,
  const char*              message);

/**
 * \brief Retrieves the last error message recorded for a specific unit_interval.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_unit_interval_set_last_error for the given
 * unit_interval. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] unit_interval A pointer to the \ref cardano_unit_interval_t instance whose last error
 *                   message is to be retrieved. If the unit_interval is NULL, the function
 *                   returns a generic error message indicating the null unit_interval.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified unit_interval. If the unit_interval is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_unit_interval_set_last_error for the same unit_interval, or until
 *       the unit_interval is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_unit_interval_get_last_error(
  const cardano_unit_interval_t* unit_interval);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_UNIT_INTERVAL_H