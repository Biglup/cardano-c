/**
 * \file ex_units.h
 *
 * \author angel.castillo
 * \date   Mar 06, 2024
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_EX_UNITS_H
#define BIGLUP_LABS_INCLUDE_CARDANO_EX_UNITS_H

/* INCLUDES ******************************************************************/

#include <cardano/cbor/cbor_reader.h>
#include <cardano/cbor/cbor_writer.h>
#include <cardano/crypto/blake2b_hash.h>
#include <cardano/error.h>
#include <cardano/export.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Represent a measure of computational resources, specifically, how much memory
 * and CPU a Plutus script will use when executed. It's an essential component to
 * estimate the cost of running a Plutus script on the Cardano blockchain.
 *
 * The two resources measured by ExUnits are memory and CPU. When a Plutus script
 * is executed, it consumes both these resources. The ExUnits system quantifies
 * this consumption, helping to ensure that scripts don't overrun the system and
 * that they terminate in a reasonable amount of time.
 */
typedef struct cardano_ex_units_t cardano_ex_units_t;

/**
 * \brief Creates and initializes a new instance of \ref cardano_ex_units_t.
 *
 * This function allocates and initializes a new instance of \ref cardano_ex_units_t.
 * Execution units (ExUnits) are a measure of  the computational resources required.
 *
 * \param[in] memory The amount of memory (in units) that the script is expected to consume.
 * \param[in] cpu_steps The number of CPU steps that the script is expected to consume.
 * \param[out] ex_units On successful initialization, this will point to a newly created
 *            \ref cardano_ex_units_t object. This object represents a "strong reference"
 *            to the ex_units, meaning that it is fully initialized and ready for use.
 *            The caller is responsible for managing the lifecycle of this object.
 *            Specifically, once the execution units are no longer needed, the caller must release them
 *            by calling \ref cardano_ex_units_unref.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the execution units were successfully created, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_ex_units_t* ex_units = NULL;
 * uint64_t memory = 1024; // 1024 units of memory
 * uint64_t cpu_steps = 500; // 500 CPU steps
 *
 * // Attempt to create a new execution units object
 * cardano_error_t result = cardano_ex_units_new(memory, cpu_steps, &ex_units);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the ex_units
 *
 *   // Once done, ensure to clean up and release the ex_units
 *   cardano_ex_units_unref(&ex_units);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_ex_units_new(
  uint64_t             memory,
  uint64_t             cpu_steps,
  cardano_ex_units_t** ex_units);

/**
 * \brief Creates a \ref cardano_ex_units_t from a CBOR reader.
 *
 * This function parses CBOR data using a provided \ref cardano_cbor_reader_t and constructs a \ref cardano_ex_units_t object.
 * It assumes that the CBOR reader is set up correctly and that the CBOR data corresponds to the structure expected for a ex_units.
 *
 * \param[in] reader A pointer to an initialized \ref cardano_cbor_reader_t that is ready to read the CBOR-encoded data.
 * \param[out] ex_units A pointer to a pointer of \ref cardano_ex_units_t that will be set to the address
 *                        of the newly created ex_units object upon successful decoding.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the execution units were successfully created, or an appropriate error code if an error occurred.
 *
 * \note If the function fails, the last error can be retrieved by calling \ref cardano_cbor_reader_get_last_error with the reader.
 *       The caller is responsible for freeing the created \ref cardano_ex_units_t object by calling
 *       \ref cardano_ex_units_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_new(cbor_data, data_size);
 * cardano_ex_units_t* ex_units = NULL;
 *
 * cardano_error_t result = cardano_ex_units_from_cbor(reader, &ex_units);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the ex_units
 *
 *   // Once done, ensure to clean up and release the ex_units
 *   cardano_ex_units_unref(&ex_units);
 * }
 * else
 * {
 *   const char* error = cardano_cbor_reader_get_last_error(reader);
 *   printf("Failed to decode ex_units: %s\n", error);
 * }
 *
 * cardano_cbor_reader_unref(&reader); // Cleanup the CBOR reader
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_ex_units_from_cbor(cardano_cbor_reader_t* reader, cardano_ex_units_t** ex_units);

/**
 * \brief Serializes execution units into CBOR format using a CBOR writer.
 *
 * This function serializes the given \ref cardano_ex_units_t object using a \ref cardano_cbor_writer_t.
 *
 * \param[in] ex_units A constant pointer to the \ref cardano_ex_units_t object that is to be serialized.
 * \param[out] writer A pointer to a \ref cardano_cbor_writer_t where the CBOR serialized data will be written.
 *                    The writer must already be initialized and ready to accept the data.
 *
 * \return Returns \ref CARDANO_SUCCESS if the serialization is successful. If the \p ex_units or \p writer
 *         is NULL, returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_ex_units_t* ex_units = ...;
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * if (writer)
 * {
 *   cardano_error_t result = cardano_ex_units_to_cbor(ex_units, writer);
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
 * cardano_ex_units_unref(&ex_units);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_ex_units_to_cbor(
  const cardano_ex_units_t* ex_units,
  cardano_cbor_writer_t*    writer);

/**
 * \brief Retrieves the memory component of the execution units.
 *
 * \param[in] ex_units A constant pointer to the \ref cardano_ex_units_t object from which
 *                     the memory is to be retrieved.
 *
 * \return The amount of memory.
 *
 * Usage Example:
 * \code{.c}
 * cardano_ex_units_t* ex_units = ...; // Assume ex_units is initialized and contains memory and cpu_steps
 * uint64_t memory_allocated = cardano_ex_units_get_memory(ex_units);
 * printf("Memory: %llu units\n", memory_allocated);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT uint64_t cardano_ex_units_get_memory(
  const cardano_ex_units_t* ex_units);

/**
 * \brief Sets the memory amount for the execution units.
 *
 * This function sets the memory units in the \ref cardano_ex_units_t object.
 *
 * \param[in,out] ex_units A pointer to the \ref cardano_ex_units_t object whose memory amount
 *                         is to be set. This object must be previously initialized.
 * \param[in] memory The memory amount to be set for the execution units.
 *
 * \return \ref CARDANO_SUCCESS if the memory was successfully set; otherwise, an appropriate
 *         error code is returned indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_ex_units_t* ex_units = cardano_ex_units_new(1024, 1000); // Assume ex_units is initialized with default values
 * cardano_error_t result = cardano_ex_units_set_memory(ex_units, 2048); // Update memory to 2048 bytes
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Memory value updated successfully
 * }
 * else
 * {
 *   // Handle error
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_ex_units_set_memory(
  cardano_ex_units_t* ex_units,
  uint64_t            memory);

/**
 * \brief Retrieves the CPU steps allocated to an cardano_ex_units_t object.
 *
 * This function retrieves the amount of CPU steps that have been allocated to a \ref cardano_ex_units_t object,
 * representing the computational resources allowed for executing a script.
 *
 * \param[in] ex_units A constant pointer to the \ref cardano_ex_units_t object from which
 *                     the CPU steps are to be retrieved.
 *
 * \return The amount of CPU steps allocated to the cardano_ex_units_t object. If the ex_units is NULL,
 *        the function returns 0.
 *
 * Usage Example:
 * \code{.c}
 * cardano_ex_units_t* ex_units = ...; // Assume this is initialized
 * uint64_t cpu_steps = cardano_ex_units_get_cpu_steps(ex_units);
 * printf("CPU steps allocated: %llu\n", (unsigned long long)cpu_steps);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT uint64_t cardano_ex_units_get_cpu_steps(const cardano_ex_units_t* ex_units);

/**
 * \brief Sets the CPU steps for the specified execution units object.
 *
 * This function sets the number of CPU steps that the execution units object will represent.
 *
 * \param[in,out] ex_units A pointer to the \ref cardano_ex_units_t object whose CPU steps are to be set.
 * \param[in] cpu_steps The number of CPU steps to set for the execution units.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the CPU steps were successfully set, or an appropriate error code if an error occurred.
 *
 * \note It is important to ensure that the \p ex_units object has been properly initialized before
 *       calling this function. If \p ex_units is NULL, the function will return \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_ex_units_t* ex_units = ...; // Assume ex_units is already created and initialized
 * uint64_t cpu_steps = 5000; // Example CPU steps
 *
 * cardano_error_t result = cardano_ex_units_set_cpu_steps(ex_units, cpu_steps);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // CPU steps are now set to 5000
 *   // Proceed with using the ex_units object
 * }
 * else
 * {
 *   // Handle the error
 *   fprintf(stderr, "Failed to set CPU steps: %d\n", result);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_ex_units_set_cpu_steps(
  cardano_ex_units_t* ex_units,
  uint64_t            cpu_steps);

/**
 * \brief Decrements the reference count of a cardano_ex_units_t object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_ex_units_t object
 * by decreasing its reference count. When the reference count reaches zero, the ex_units is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] ex_units A pointer to the pointer of the ex_units object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_ex_units_t* ex_units = cardano_ex_units_new(mem, cpu_steps);
 *
 * // Perform operations with the ex_units...
 *
 * cardano_ex_units_unref(&ex_units);
 * // At this point, ex_units is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_ex_units_unref, the pointer to the \ref cardano_ex_units_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_ex_units_unref(cardano_ex_units_t** ex_units);

/**
 * \brief Increases the reference count of the cardano_ex_units_t object.
 *
 * This function is used to manually increment the reference count of an cardano_ex_units_t
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_ex_units_unref.
 *
 * \param ex_units A pointer to the cardano_ex_units_t object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming ex_units is a previously created ex_units object
 *
 * cardano_ex_units_ref(ex_units);
 *
 * // Now ex_units can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_ex_units_ref there is a corresponding
 * call to \ref cardano_ex_units_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_ex_units_ref(cardano_ex_units_t* ex_units);

/**
 * \brief Retrieves the current reference count of the cardano_ex_units_t object.
 *
 * This function returns the number of active references to an cardano_ex_units_t object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_ex_units_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param ex_units A pointer to the cardano_ex_units_t object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified cardano_ex_units_t object. If the object
 * is properly managed (i.e., every \ref cardano_ex_units_ref call is matched with a
 * \ref cardano_ex_units_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming ex_units is a previously created ex_units object
 *
 * size_t ref_count = cardano_ex_units_refcount(ex_units);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_ex_units_refcount(const cardano_ex_units_t* ex_units);

/**
 * \brief Sets the last error message for a given cardano_ex_units_t object.
 *
 * Records an error message in the ex_units's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] ex_units A pointer to the \ref cardano_ex_units_t instance whose last error message is
 *                       to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the ex_units's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_ex_units_set_last_error(cardano_ex_units_t* ex_units, const char* message);

/**
 * \brief Retrieves the last error message recorded for a specific ex_units.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_ex_units_set_last_error for the given
 * ex_units. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] ex_units A pointer to the \ref cardano_ex_units_t instance whose last error
 *                   message is to be retrieved. If the ex_units is NULL, the function
 *                   returns a generic error message indicating the null ex_units.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified ex_units. If the ex_units is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_ex_units_set_last_error for the same ex_units, or until
 *       the ex_units is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_ex_units_get_last_error(const cardano_ex_units_t* ex_units);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_EX_UNITS_H