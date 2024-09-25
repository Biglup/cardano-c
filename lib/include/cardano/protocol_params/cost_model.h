/**
 * \file cost_model.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_COST_MODEL_H
#define BIGLUP_LABS_INCLUDE_CARDANO_COST_MODEL_H

/* INCLUDES ******************************************************************/

#include <cardano/cbor/cbor_reader.h>
#include <cardano/cbor/cbor_writer.h>
#include <cardano/error.h>
#include <cardano/export.h>
#include <cardano/scripts/plutus_scripts/plutus_language_version.h>
#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief The execution of plutus scripts consumes resources. To make sure that these
 * scripts don't run indefinitely or consume excessive resources (which would be
 * harmful to the network), Cardano introduces the concept of "cost models".
 *
 * Cost models are in place to provide predictable pricing for script execution.
 *
 * It's a way to gauge how much resource (in terms of computational steps or memory)
 * a script would use.
 */
typedef struct cardano_cost_model_t cardano_cost_model_t;

/**
 * \brief Creates and initializes a new instance of the Cost Model for a specific Plutus language version.
 *
 * This function allocates and initializes a new instance of the Cost Model based on the specified Plutus language version,
 * which is used to predict and control the computational cost of executing Plutus scripts on the Cardano blockchain.
 *
 * \param[in] language The Plutus language version for which the cost model is being created.
 * \param[in] cost_array An array of integers representing the cost associated with each operation in the Plutus script.
 * \param[in] costs_size The size of the cost_array, indicating the number of operations covered by the cost model.
 * \param[out] cost_model On successful initialization, this will point to a newly created
 *                        \ref cardano_cost_model_t object. This object represents a "strong reference",
 *                        meaning that it is fully initialized and ready for use.
 *                        The caller is responsible for managing the lifecycle of this object.
 *                        Specifically, once the Cost Model is no longer needed, the caller must release it
 *                        by calling \ref cardano_cost_model_unref.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the Cost Model was successfully created, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cost_model_t* cost_model = NULL;
 * cardano_plutus_language_version_t language = CARDANO_PLUTUS_LANGUAGE_VERSION_V1;
 * int64_t cost_array[] = { 1, 2, 3 ... }; // Example costs for operations
 * size_t costs_size = sizeof(cost_array) / sizeof(cost_array[0]);
 *
 * cardano_error_t result = cardano_cost_model_new(language, cost_array, costs_size, &cost_model);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the cost_model
 *
 *   // Once done, ensure to clean up and release the cost_model
 *   cardano_cost_model_unref(&cost_model);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_cost_model_new(
  cardano_plutus_language_version_t language,
  const int64_t*                    cost_array,
  size_t                            costs_size,
  cardano_cost_model_t**            cost_model);

/**
 * \brief Creates a \ref cardano_cost_model_t from a CBOR reader.
 *
 * This function parses CBOR data using a provided \ref cardano_cbor_reader_t and constructs a \ref cardano_cost_model_t object.
 * It assumes that the CBOR reader is set up correctly and that the CBOR data corresponds to the structure expected for a cost_model.
 *
 * \param[in] reader A pointer to an initialized \ref cardano_cbor_reader_t that is ready to read the CBOR-encoded data.
 * \param[out] cost_model A pointer to a pointer of \ref cardano_cost_model_t that will be set to the address
 *                        of the newly created cost_model object upon successful decoding.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the Cost Model were successfully created, or an appropriate error code if an error occurred.
 *
 * \note If the function fails, the last error can be retrieved by calling \ref cardano_cbor_reader_get_last_error with the reader.
 *       The caller is responsible for freeing the created \ref cardano_cost_model_t object by calling
 *       \ref cardano_cost_model_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_new(cbor_data, data_size);
 * cardano_cost_model_t* cost_model = NULL;
 *
 * cardano_error_t result = cardano_cost_model_from_cbor(reader, &cost_model);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the cost_model
 *
 *   // Once done, ensure to clean up and release the cost_model
 *   cardano_cost_model_unref(&cost_model);
 * }
 * else
 * {
 *   const char* error = cardano_cbor_reader_get_last_error(reader);
 *   printf("Failed to decode cost_model: %s\n", error);
 * }
 *
 * cardano_cbor_reader_unref(&reader); // Cleanup the CBOR reader
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_cost_model_from_cbor(cardano_cbor_reader_t* reader, cardano_cost_model_t** cost_model);

/**
 * \brief Serializes Cost Model into CBOR format using a CBOR writer.
 *
 * This function serializes the given \ref cardano_cost_model_t object using a \ref cardano_cbor_writer_t.
 *
 * \param[in] cost_model A constant pointer to the \ref cardano_cost_model_t object that is to be serialized.
 * \param[out] writer A pointer to a \ref cardano_cbor_writer_t where the CBOR serialized data will be written.
 *                    The writer must already be initialized and ready to accept the data.
 *
 * \return Returns \ref CARDANO_SUCCESS if the serialization is successful. If the \p cost_model or \p writer
 *         is NULL, returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cost_model_t* cost_model = ...;
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * if (writer)
 * {
 *   cardano_error_t result = cardano_cost_model_to_cbor(cost_model, writer);
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
 * cardano_cost_model_unref(&cost_model);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_cost_model_to_cbor(
  const cardano_cost_model_t* cost_model,
  cardano_cbor_writer_t*      writer);

/**
 * \brief Sets the cost for a specific operation in the Cost Model.
 *
 * This function updates the Cost Model with the cost for a specific operation. It modifies the
 * cost associated with an operation in the array of costs maintained by the Cost Model object.
 *
 * \param[in] cost_model Pointer to the Cost Model object.
 * \param[in] operation The index of the operation for which the cost is to be set.
 * \param[in] cost The cost value to be set for the specified operation.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the cost was successfully set, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cost_model_t* cost_model = NULL;
 * // Assume cost_model is initialized properly
 *
 * size_t operation = 5;  // Example operation index
 * int64_t cost = 100;    // Example cost for the operation
 * cardano_error_t result = cardano_cost_model_set_cost(cost_model, operation, cost);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *     printf("Cost for operation %zu set successfully.\n", operation);
 * }
 * else
 * {
 *     printf("Failed to set cost for operation %zu.\n", operation);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_cost_model_set_cost(
  cardano_cost_model_t* cost_model,
  size_t                operation,
  int64_t               cost);

/**
 * \brief Retrieves the cost associated with a specific operation in the Cost Model.
 *
 * This function fetches the cost of a specific operation as defined in the Cost Model.
 * The cost is stored in the variable pointed to by the `cost` parameter.
 *
 * \param[in] cost_model Pointer to the Cost Model object.
 * \param[in] operation The index of the operation whose cost is to be retrieved.
 * \param[out] cost Pointer to an integer where the cost of the operation will be stored.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the cost was successfully retrieved, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cost_model_t* cost_model = NULL;
 * // Assume cost_model is initialized properly
 *
 * size_t operation = 5;  // Example operation index
 * int64_t cost;          // Variable to store the retrieved cost
 * cardano_error_t result = cardano_cost_model_get_cost(cost_model, operation, &cost);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *     printf("Cost for operation %zu is %ld.\n", operation, cost);
 * }
 * else
 * {
 *     printf("Failed to retrieve cost for operation %zu.\n", operation);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_cost_model_get_cost(const cardano_cost_model_t* cost_model, size_t operation, int64_t* cost);

/**
 * \brief Retrieves the size of the costs array in the Cost Model.
 *
 * This function returns the number of operations and their associated costs stored in the Cost Model,
 * effectively indicating the number of different operational costs the model covers.
 *
 * \param[in] cost_model Pointer to the Cost Model object.
 *
 * \return The number of entries in the cost model's costs array, representing the total number of operations
 *         for which costs are defined.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cost_model_t* cost_model = NULL;
 * // Assume cost_model is initialized properly
 *
 * size_t costs_size = cardano_cost_model_get_costs_size(cost_model);
 * printf("The cost model contains costs for %zu operations.\n", costs_size);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_cost_model_get_costs_size(const cardano_cost_model_t* cost_model);

/**
 * \brief Retrieves a pointer to the costs array in the Cost Model.
 *
 * This function provides direct read-only access to the internal array representing the costs of operations
 * within the Cost Model. This array contains the computational costs associated with various operations as
 * defined by the Plutus script execution environment.
 *
 * \param[in] cost_model Pointer to the Cost Model object.
 *
 * \return A constant pointer to the first element of the internal costs array. The size of this array can
 *         be determined using \ref cardano_cost_model_get_costs_size function.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cost_model_t* cost_model = NULL;
 * // Assume cost_model is initialized properly
 *
 * const int64_t* costs = cardano_cost_model_get_costs(cost_model);
 * size_t size = cardano_cost_model_get_costs_size(cost_model);
 *
 * for (size_t i = 0; i < size; ++i)
 * {
 *   printf("Cost of operation %zu: %ld\n", i, costs[i]);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT const int64_t* cardano_cost_model_get_costs(const cardano_cost_model_t* cost_model);

/**
 * \brief Retrieves the Plutus language version of the Cost Model.
 *
 * This function returns the Plutus language version used by the Cost Model. The Plutus language version
 * determines the set of operations and their associated costs specific to a version of the Cardano protocol
 * for executing Plutus scripts.
 *
 * \param[in]  cost_model Pointer to the Cost Model object.
 * \param[out] language   Pointer to a variable where the language version will be stored upon successful return.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the language version was successfully retrieved, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cost_model_t* cost_model = NULL;
 * cardano_plutus_language_version_t language;
 *
 * // Assume cost_model is initialized properly
 * cardano_error_t result = cardano_cost_model_get_language(cost_model, &language);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Plutus Language Version: %d\n", language);
 * }
 * else
 * {
 *   printf("Failed to retrieve language version.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_cost_model_get_language(
  const cardano_cost_model_t*        cost_model,
  cardano_plutus_language_version_t* language);

/**
 * \brief Decrements the reference count of a cardano_cost_model_t object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_cost_model_t object
 * by decreasing its reference count. When the reference count reaches zero, the cost_model is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] cost_model A pointer to the pointer of the cost_model object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cost_model_t* cost_model = cardano_cost_model_new(major, minor);
 *
 * // Perform operations with the cost_model...
 *
 * cardano_cost_model_unref(&cost_model);
 * // At this point, cost_model is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_cost_model_unref, the pointer to the \ref cardano_cost_model_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_cost_model_unref(cardano_cost_model_t** cost_model);

/**
 * \brief Increases the reference count of the cardano_cost_model_t object.
 *
 * This function is used to manually increment the reference count of an cardano_cost_model_t
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_cost_model_unref.
 *
 * \param cost_model A pointer to the cardano_cost_model_t object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming cost_model is a previously created cost_model object
 *
 * cardano_cost_model_ref(cost_model);
 *
 * // Now cost_model can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_cost_model_ref there is a corresponding
 * call to \ref cardano_cost_model_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_cost_model_ref(cardano_cost_model_t* cost_model);

/**
 * \brief Retrieves the current reference count of the cardano_cost_model_t object.
 *
 * This function returns the number of active references to an cardano_cost_model_t object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_cost_model_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param cost_model A pointer to the cardano_cost_model_t object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified cardano_cost_model_t object. If the object
 * is properly managed (i.e., every \ref cardano_cost_model_ref call is matched with a
 * \ref cardano_cost_model_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming cost_model is a previously created cost_model object
 *
 * size_t ref_count = cardano_cost_model_refcount(cost_model);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_cost_model_refcount(const cardano_cost_model_t* cost_model);

/**
 * \brief Sets the last error message for a given cardano_cost_model_t object.
 *
 * Records an error message in the cost_model's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] cost_model A pointer to the \ref cardano_cost_model_t instance whose last error message is
 *                       to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the cost_model's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_cost_model_set_last_error(
  cardano_cost_model_t* cost_model,
  const char*           message);

/**
 * \brief Retrieves the last error message recorded for a specific cost_model.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_cost_model_set_last_error for the given
 * cost_model. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] cost_model A pointer to the \ref cardano_cost_model_t instance whose last error
 *                   message is to be retrieved. If the cost_model is NULL, the function
 *                   returns a generic error message indicating the null cost_model.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified cost_model. If the cost_model is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_cost_model_set_last_error for the same cost_model, or until
 *       the cost_model is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_cost_model_get_last_error(
  const cardano_cost_model_t* cost_model);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_COST_MODEL_H