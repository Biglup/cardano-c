/**
 * \file costmdls.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_COSTMDLS_H
#define BIGLUP_LABS_INCLUDE_CARDANO_COSTMDLS_H

/* INCLUDES ******************************************************************/

#include <cardano/cbor/cbor_reader.h>
#include <cardano/cbor/cbor_writer.h>
#include <cardano/error.h>
#include <cardano/export.h>
#include <cardano/protocol_params/cost_model.h>
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
typedef struct cardano_costmdls_t cardano_costmdls_t;

/**
 * \brief Initializes a new instance of the \ref cardano_costmdls_t type.
 *
 * This function allocates and initializes a new instance of the \ref cardano_costmdls_t type,
 * which holds a map of Plutus language versions to their respective Cost Models.
 *
 * \param[out] costmdls On successful initialization, this will point to a newly created
 *            \ref cardano_costmdls_t object. This object represents a "strong reference",
 *            meaning that it is fully initialized and ready for use.
 *            The caller is responsible for managing the lifecycle of this object.
 *            Specifically, once the \ref cardano_costmdls_t is no longer needed, the caller must release it
 *            by calling \ref cardano_costmdls_unref.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the \ref cardano_costmdls_t was successfully created, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_costmdls_t* costmdls = NULL;
 *
 * // Attempt to create a new cardano_costmdls_t object
 * cardano_error_t result = cardano_costmdls_new(&costmdls);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the costmdls
 *
 *   // Once done, ensure to clean up and release the costmdls
 *   cardano_costmdls_unref(&costmdls);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_costmdls_new(cardano_costmdls_t** costmdls);

/**
 * \brief Creates a \ref cardano_costmdls_t from a CBOR reader.
 *
 * This function parses CBOR data using a provided \ref cardano_cbor_reader_t and constructs a \ref cardano_costmdls_t object.
 * It assumes that the CBOR reader is set up correctly and that the CBOR data corresponds to the structure expected for a costmdls.
 *
 * \param[in] reader A pointer to an initialized \ref cardano_cbor_reader_t that is ready to read the CBOR-encoded data.
 * \param[out] costmdls A pointer to a pointer of \ref cardano_costmdls_t that will be set to the address
 *                        of the newly created costmdls object upon successful decoding.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the Cost Model were successfully created, or an appropriate error code if an error occurred.
 *
 * \note If the function fails, the last error can be retrieved by calling \ref cardano_cbor_reader_get_last_error with the reader.
 *       The caller is responsible for freeing the created \ref cardano_costmdls_t object by calling
 *       \ref cardano_costmdls_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_new(cbor_data, data_size);
 * cardano_costmdls_t* costmdls = NULL;
 *
 * cardano_error_t result = cardano_costmdls_from_cbor(reader, &costmdls);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the costmdls
 *
 *   // Once done, ensure to clean up and release the costmdls
 *   cardano_costmdls_unref(&costmdls);
 * }
 * else
 * {
 *   const char* error = cardano_cbor_reader_get_last_error(reader);
 *   printf("Failed to decode costmdls: %s\n", error);
 * }
 *
 * cardano_cbor_reader_unref(&reader); // Cleanup the CBOR reader
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_costmdls_from_cbor(cardano_cbor_reader_t* reader, cardano_costmdls_t** costmdls);

/**
 * \brief Serializes Cost Model into CBOR format using a CBOR writer.
 *
 * This function serializes the given \ref cardano_costmdls_t object using a \ref cardano_cbor_writer_t.
 *
 * \param[in] costmdls A constant pointer to the \ref cardano_costmdls_t object that is to be serialized.
 * \param[out] writer A pointer to a \ref cardano_cbor_writer_t where the CBOR serialized data will be written.
 *                    The writer must already be initialized and ready to accept the data.
 *
 * \return Returns \ref CARDANO_SUCCESS if the serialization is successful. If the \p costmdls or \p writer
 *         is NULL, returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_costmdls_t* costmdls = ...;
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * if (writer)
 * {
 *   cardano_error_t result = cardano_costmdls_to_cbor(costmdls, writer);
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
 * cardano_costmdls_unref(&costmdls);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_costmdls_to_cbor(
  const cardano_costmdls_t* costmdls,
  cardano_cbor_writer_t*    writer);

/**
 * \brief Inserts a new Cost Model into the Costmdls map.
 *
 * This function inserts a new Cost Model into the Costmdls map. Each Cost Model is associated with a specific
 * version of the Plutus language and contains the computational cost parameters used for script execution.
 *
 * \param[in] costmdls Pointer to the Costmdls object.
 * \param[in] cost_model Pointer to the Cost Model object to be inserted.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the Cost Model was successfully inserted, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_costmdls_t* costmdls = NULL;
 * cardano_cost_model_t* cost_model = NULL;
 * // Assume costmdls and cost_model are initialized properly
 *
 * cardano_error_t result = cardano_costmdls_insert(costmdls, cost_model);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Cost Model inserted successfully.\n");
 * }
 * else
 * {
 *   printf("Failed to insert Cost Model.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_costmdls_insert(
  cardano_costmdls_t*   costmdls,
  cardano_cost_model_t* cost_model);

/**
 * \brief Retrieves a specific Cost Model from the Costmdls map based on the Plutus language version.
 *
 * This function retrieves a Cost Model corresponding to a specific version of the Plutus language from
 * the Costmdls map. It provides access to the computational cost parameters that are used for script execution
 * for the specified language version.
 *
 * \param[in] costmdls Pointer to the Costmdls object from which the Cost Model is to be retrieved.
 * \param[in] language The Plutus language version for which the Cost Model is required.
 * \param[out] cost_model On successful retrieval, this will point to the Cost Model object associated with
 *             the specified language version. The object represents a "strong reference" to the Cost Model,
 *             meaning that it remains valid until the caller releases it using \ref cardano_cost_model_unref.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the Cost Model was successfully retrieved, or an appropriate error code
 *         indicating the failure reason (e.g., if no model exists for the specified language version).
 *
 * Usage Example:
 * \code{.c}
 * cardano_costmdls_t* costmdls = NULL;
 * cardano_cost_model_t* cost_model = NULL;
 * // Assume costmdls is initialized properly
 *
 * cardano_error_t result = cardano_costmdls_get(costmdls, CARDANO_PLUTUS_LANGUAGE_VERSION_V1, &cost_model);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the cost_model for computations
 *   // Remember to release the cost_model after use
 *   cardano_cost_model_unref(&cost_model);
 * }
 * else
 * {
 *   printf("Failed to retrieve Cost Model for Plutus V1.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_costmdls_get(
  cardano_costmdls_t*               costmdls,
  cardano_plutus_language_version_t language,
  cardano_cost_model_t**            cost_model);

/**
 * \brief Checks if a Cost Model for a specific Plutus language version exists in the Costmdls map.
 *
 * This function determines whether there is a Cost Model associated with a specific version of the
 * Plutus language in the provided Costmdls map. It's useful for validating the presence of cost
 * models before attempting to retrieve or use them in computations.
 *
 * \param[in] costmdls Pointer to the Costmdls object to be checked.
 * \param[in] language The Plutus language version to check for.
 *
 * \return \c true if a Cost Model for the specified language version exists in the Costmdls map,
 *         \c false otherwise.
 *
 * Usage Example:
 * \code{.c}
 * cardano_costmdls_t* costmdls = NULL;
 * // Assume costmdls is initialized properly
 *
 * bool exists = cardano_costmdls_has(costmdls, CARDANO_PLUTUS_V1);
 *
 * if (exists)
 * {
 *   printf("Cost Model for Plutus V1 exists.\n");
 * }
 * else
 * {
 *   printf("Cost Model for Plutus V1 does not exist.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT bool cardano_costmdls_has(
  const cardano_costmdls_t*         costmdls,
  cardano_plutus_language_version_t language);

/**
 * \brief Retrieves the language views encoding from the Costmdls object.
 *
 * This function encodes the costs models following the CDDL specification, necessary
 * for computing the script data hash of a transaction. It encodes the costs associated
 * with each Plutus language version in the Costmdls map in the appropriate CBOR format,
 * following the rules defined for each version.
 *
 * \param[in] costmdls Pointer to the Costmdls object whose language views are to be encoded.
 * \param[out] language_views On successful encoding, this will point to a buffer containing
 *                            the encoded language views. The caller is responsible for managing
 *                            the lifecycle of this buffer, which includes releasing it
 *                            using \ref cardano_buffer_unref when it is no longer needed.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the language views were successfully encoded, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_costmdls_t* costmdls = NULL;
 * // Assume costmdls is initialized and contains cost models for various language versions
 * cardano_buffer_t* language_views = NULL;
 *
 * cardano_error_t result = cardano_costmdls_get_language_views_encoding(costmdls, &language_views);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the language_views buffer
 *
 *   // Once done, ensure to clean up and release the language_views
 *   cardano_buffer_unref(&language_views);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_costmdls_get_language_views_encoding(
  const cardano_costmdls_t* costmdls,
  cardano_buffer_t**        language_views);

/**
 * \brief Decrements the reference count of a cardano_costmdls_t object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_costmdls_t object
 * by decreasing its reference count. When the reference count reaches zero, the costmdls is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] costmdls A pointer to the pointer of the costmdls object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_costmdls_t* costmdls = cardano_costmdls_new(major, minor);
 *
 * // Perform operations with the costmdls...
 *
 * cardano_costmdls_unref(&costmdls);
 * // At this point, costmdls is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_costmdls_unref, the pointer to the \ref cardano_costmdls_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_costmdls_unref(cardano_costmdls_t** costmdls);

/**
 * \brief Increases the reference count of the cardano_costmdls_t object.
 *
 * This function is used to manually increment the reference count of an cardano_costmdls_t
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_costmdls_unref.
 *
 * \param costmdls A pointer to the cardano_costmdls_t object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming costmdls is a previously created costmdls object
 *
 * cardano_costmdls_ref(costmdls);
 *
 * // Now costmdls can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_costmdls_ref there is a corresponding
 * call to \ref cardano_costmdls_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_costmdls_ref(cardano_costmdls_t* costmdls);

/**
 * \brief Retrieves the current reference count of the cardano_costmdls_t object.
 *
 * This function returns the number of active references to an cardano_costmdls_t object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_costmdls_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param costmdls A pointer to the cardano_costmdls_t object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified cardano_costmdls_t object. If the object
 * is properly managed (i.e., every \ref cardano_costmdls_ref call is matched with a
 * \ref cardano_costmdls_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming costmdls is a previously created costmdls object
 *
 * size_t ref_count = cardano_costmdls_refcount(costmdls);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_costmdls_refcount(const cardano_costmdls_t* costmdls);

/**
 * \brief Sets the last error message for a given cardano_costmdls_t object.
 *
 * Records an error message in the costmdls's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] costmdls A pointer to the \ref cardano_costmdls_t instance whose last error message is
 *                       to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the costmdls's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_costmdls_set_last_error(
  cardano_costmdls_t* costmdls,
  const char*         message);

/**
 * \brief Retrieves the last error message recorded for a specific costmdls.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_costmdls_set_last_error for the given
 * costmdls. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] costmdls A pointer to the \ref cardano_costmdls_t instance whose last error
 *                   message is to be retrieved. If the costmdls is NULL, the function
 *                   returns a generic error message indicating the null costmdls.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified costmdls. If the costmdls is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_costmdls_set_last_error for the same costmdls, or until
 *       the costmdls is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_costmdls_get_last_error(
  const cardano_costmdls_t* costmdls);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_COSTMDLS_H