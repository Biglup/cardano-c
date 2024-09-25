/**
 * \file redeemer.h
 *
 * \author angel.castillo
 * \date   Sep 20, 2024
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_REDEEMER_H
#define BIGLUP_LABS_INCLUDE_CARDANO_REDEEMER_H

/* INCLUDES ******************************************************************/

#include <cardano/cbor/cbor_reader.h>
#include <cardano/cbor/cbor_writer.h>
#include <cardano/common/ex_units.h>
#include <cardano/error.h>
#include <cardano/export.h>
#include <cardano/plutus_data/plutus_data.h>
#include <cardano/typedefs.h>
#include <cardano/witness_set/redeemer_tag.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * The Redeemer is an argument provided to a Plutus smart contract (script) when
 * you are attempting to redeem a UTxO that's protected by that script.
 */
typedef struct cardano_redeemer_t cardano_redeemer_t;

/**
 * \brief Creates and initializes a new instance of a Redeemer.
 *
 * This function allocates and initializes a new instance of a \ref cardano_redeemer_t object,
 * representing a redeemer in the Cardano protocol. A redeemer is used to provide data for script execution
 * in transactions. The redeemer includes a tag, an index, data, and execution units (ex_units) required for script validation.
 *
 * \param[in] tag The \ref cardano_redeemer_tag_t representing the type of action (e.g., spending, minting, reward) that the redeemer is associated with.
 * \param[in] index The index of the transaction input this redeemer is intended for. The transaction inputs are indexed in the map order by their transaction id.
 * \param[in] data A pointer to a \ref cardano_plutus_data_t object containing the Plutus data associated with this redeemer. This data is required for script execution.
 * \param[in] ex_units A pointer to a \ref cardano_ex_units_t object representing the execution units (computation and memory) allocated for this redeemer.
 *                     This defines how much computational and memory resources are available for the script execution.
 * \param[out] redeemer On successful initialization, this will point to a newly created \ref cardano_redeemer_t object.
 *                      This object represents a "strong reference", meaning that it is fully initialized and ready for use.
 *                      The caller is responsible for managing the lifecycle of this object. Specifically, once the redeemer is no longer needed,
 *                      the caller must release it by calling \ref cardano_redeemer_unref.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the redeemer was successfully created, or an appropriate error code indicating the failure reason.
 *
 * \note The caller is responsible for ensuring that both the \p data and \p ex_units parameters are valid and initialized before calling this function.
 *       Additionally, the caller must manage the memory of these parameters.
 *
 * Usage Example:
 * \code{.c}
 * cardano_redeemer_t* redeemer = NULL;
 * cardano_plutus_data_t* data = ...;  // Assume data is initialized
 * cardano_ex_units_t* ex_units = ...;  // Assume ex_units is initialized
 * cardano_redeemer_tag_t tag = CARDANO_REDEEMER_TAG_SPENDING;  // Example tag
 * uint64_t index = 0;  // Example index
 *
 * // Attempt to create a new Redeemer object
 * cardano_error_t result = cardano_redeemer_new(tag, index, data, ex_units, &redeemer);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the redeemer
 *
 *   // Once done, ensure to clean up and release the redeemer
 *   cardano_redeemer_unref(&redeemer);
 * }
 * else
 * {
 *   printf("Failed to create the redeemer: %s\n", cardano_error_to_string(result));
 * }
 *
 * // Clean up data and ex_units as necessary
 * cardano_plutus_data_unref(&data);
 * cardano_ex_units_unref(&ex_units);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_redeemer_new(
  cardano_redeemer_tag_t tag,
  uint64_t               index,
  cardano_plutus_data_t* data,
  cardano_ex_units_t*    ex_units,
  cardano_redeemer_t**   redeemer);

/**
 * \brief Creates a \ref cardano_redeemer_t from a CBOR reader.
 *
 * This function parses CBOR data using a provided \ref cardano_cbor_reader_t and constructs a \ref cardano_redeemer_t object.
 * It assumes that the CBOR reader is set up correctly and that the CBOR data corresponds to the structure expected for a redeemer.
 *
 * \param[in] reader A pointer to an initialized \ref cardano_cbor_reader_t that is ready to read the CBOR-encoded data.
 * \param[out] redeemer A pointer to a pointer of \ref cardano_redeemer_t that will be set to the address
 *                        of the newly created redeemer object upon successful decoding.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the protocol version were successfully created, or an appropriate error code if an error occurred.
 *
 * \remark In Cardano, entities are encoded in CBOR, but CBOR allows multiple valid ways to encode the same data. The Cardano blockchain
 *         does not enforce a canonical CBOR representation, meaning that if you decode a transaction from CBOR and then re-encode it,
 *         the resulting encoding could be different. This would change the redeemer and invalidate any existing signatures.
 *         To prevent this, when a redeemer object is created using \ref cardano_redeemer_from_cbor, it caches the original
 *         CBOR representation internally. When \ref cardano_redeemer_to_cbor is called, it will output the cached CBOR.
 *         If the cached CBOR representation is not needed, the client can call \ref cardano_redeemer_clear_cbor_cache after the object has been created.
 *
 * \note If the function fails, the last error can be retrieved by calling \ref cardano_cbor_reader_get_last_error with the reader.
 *       The caller is responsible for freeing the created \ref cardano_redeemer_t object by calling
 *       \ref cardano_redeemer_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_new(cbor_data, data_size);
 * cardano_redeemer_t* redeemer = NULL;
 *
 * cardano_error_t result = cardano_redeemer_from_cbor(reader, &redeemer);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the redeemer
 *
 *   // Once done, ensure to clean up and release the redeemer
 *   cardano_redeemer_unref(&redeemer);
 * }
 * else
 * {
 *   const char* error = cardano_cbor_reader_get_last_error(reader);
 *   printf("Failed to decode redeemer: %s\n", error);
 * }
 *
 * cardano_cbor_reader_unref(&reader); // Cleanup the CBOR reader
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_redeemer_from_cbor(cardano_cbor_reader_t* reader, cardano_redeemer_t** redeemer);

/**
 * \brief Serializes protocol version into CBOR format using a CBOR writer.
 *
 * This function serializes the given \ref cardano_redeemer_t object using a \ref cardano_cbor_writer_t.
 *
 * \param[in] redeemer A constant pointer to the \ref cardano_redeemer_t object that is to be serialized.
 * \param[out] writer A pointer to a \ref cardano_cbor_writer_t where the CBOR serialized data will be written.
 *                    The writer must already be initialized and ready to accept the data.
 *
 * \return Returns \ref CARDANO_SUCCESS if the serialization is successful. If the \p redeemer or \p writer
 *         is NULL, returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * \remark In Cardano, entities are encoded in CBOR, but CBOR allows multiple valid ways to encode the same data. The Cardano blockchain
 *         does not enforce a canonical CBOR representation, meaning that if you decode a transaction from CBOR and then re-encode it,
 *         the resulting encoding could be different. This would change the redeemer and invalidate any existing signatures.
 *         To prevent this, when a redeemer object is created using \ref cardano_redeemer_from_cbor, it caches the original
 *         CBOR representation internally. When \ref cardano_redeemer_to_cbor is called, it will output the cached CBOR.
 *         If the cached CBOR representation is not needed, the client can call \ref cardano_redeemer_clear_cbor_cache after the object has been created.
 *
 * Usage Example:
 * \code{.c}
 * cardano_redeemer_t* redeemer = ...;
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * if (writer)
 * {
 *   cardano_error_t result = cardano_redeemer_to_cbor(redeemer, writer);
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
 * cardano_redeemer_unref(&redeemer);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_redeemer_to_cbor(
  const cardano_redeemer_t* redeemer,
  cardano_cbor_writer_t*    writer);

/**
 * \brief Retrieves the tag associated with a redeemer.
 *
 * This function fetches the tag from a given \ref cardano_redeemer_t object. The tag specifies the type
 * of action associated with the redeemer, such as spending, minting, or rewarding.
 *
 * \param[in] redeemer A constant pointer to an initialized \ref cardano_redeemer_t object.
 *
 * \return The \ref cardano_redeemer_tag_t indicating the type of action for the redeemer.
 *
 * Usage Example:
 * \code{.c}
 * const cardano_redeemer_t* redeemer = ...;  // Assume redeemer is already initialized
 * cardano_redeemer_tag_t tag = cardano_redeemer_get_tag(redeemer);
 *
 * if (tag == CARDANO_REDEEMER_TAG_SPENDING)
 * {
 *     printf("Redeemer tag: Spending\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_redeemer_tag_t cardano_redeemer_get_tag(const cardano_redeemer_t* redeemer);

/**
 * \brief Sets the tag for a redeemer.
 *
 * This function sets the tag of a given \ref cardano_redeemer_t object. The tag specifies the type of
 * action associated with the redeemer, such as spending, minting, or rewarding.
 *
 * \param[in,out] redeemer A pointer to an initialized \ref cardano_redeemer_t object where the tag will be set.
 * \param[in] tag A \ref cardano_redeemer_tag_t representing the new tag to be assigned to the redeemer.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the tag was
 *         successfully set, or an appropriate error code indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_redeemer_t* redeemer = ...;  // Assume redeemer is already initialized
 * cardano_error_t result = cardano_redeemer_set_tag(redeemer, CARDANO_REDEEMER_TAG_SPENDING);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *     printf("Redeemer tag set to Spending\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_redeemer_set_tag(cardano_redeemer_t* redeemer, cardano_redeemer_tag_t tag);

/**
 * \brief Retrieves the index associated with a redeemer.
 *
 * This function fetches the index from a given \ref cardano_redeemer_t object. The index represents the position
 * of the input or output in the transaction that the redeemer applies to.
 *
 * \param[in] redeemer A constant pointer to an initialized \ref cardano_redeemer_t object.
 *
 * \return A uint64_t representing the index of the redeemer.
 *
 * Usage Example:
 * \code{.c}
 * const cardano_redeemer_t* redeemer = ...;  // Assume redeemer is already initialized
 * uint64_t index = cardano_redeemer_get_index(redeemer);
 * printf("Redeemer index: %llu\n", index);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT uint64_t cardano_redeemer_get_index(const cardano_redeemer_t* redeemer);

/**
 * \brief Sets the index for the given redeemer.
 *
 * This function assigns a new index to the specified \ref cardano_redeemer_t object. The index typically refers
 * to the position of the redeemer within a list of redeemers in a Cardano transaction.
 *
 * \param[in,out] redeemer A pointer to an initialized \ref cardano_redeemer_t object to which the index will be set.
 * \param[in] index The new index to be assigned to the redeemer.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the index was
 *         successfully set, or an appropriate error code indicating the failure reason, such as \ref CARDANO_ERROR_POINTER_IS_NULL
 *         if the \p redeemer pointer is NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_redeemer_t* redeemer = ...; // Assume redeemer is initialized
 * uint64_t new_index = 3; // New index to assign
 *
 * cardano_error_t result = cardano_redeemer_set_index(redeemer, new_index);
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Index set successfully.\n");
 * }
 * else
 * {
 *   printf("Failed to set the index.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_redeemer_set_index(cardano_redeemer_t* redeemer, uint64_t index);

/**
 * \brief Retrieves the Plutus data associated with a redeemer.
 *
 * This function returns the Plutus data stored in the specified \ref cardano_redeemer_t object. The Plutus data
 * represents the input data provided to a Plutus script during transaction execution.
 *
 * \param[in] redeemer A constant pointer to an initialized \ref cardano_redeemer_t object.
 *
 * \return A pointer to the \ref cardano_plutus_data_t object representing the Plutus data.
 *         The returned Plutus data is a new reference, and the caller is responsible for managing its lifecycle
 *         by calling \ref cardano_plutus_data_unref when it is no longer needed. If the \p redeemer is NULL,
 *         this function returns NULL.
 *
 * Usage Example:
 * \code{.c}
 * const cardano_redeemer_t* redeemer = ...; // Assume redeemer is initialized
 * cardano_plutus_data_t* data = cardano_redeemer_get_data(redeemer);
 *
 * if (data != NULL)
 * {
 *   // Use the Plutus data
 *   cardano_plutus_data_unref(&data); // Ensure to release the data once done
 * }
 * else
 * {
 *   printf("Failed to retrieve Plutus data or redeemer is NULL.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_plutus_data_t* cardano_redeemer_get_data(cardano_redeemer_t* redeemer);

/**
 * \brief Sets the Plutus data for a redeemer.
 *
 * This function assigns a new \ref cardano_plutus_data_t object to the specified \ref cardano_redeemer_t.
 * The Plutus data represents the input provided to a Plutus script during transaction execution.
 *
 * \param[in,out] redeemer A pointer to an initialized \ref cardano_redeemer_t object.
 * \param[in] data A pointer to an initialized \ref cardano_plutus_data_t object that contains the new Plutus data.
 *                 This pointer can be NULL if the existing Plutus data is to be unset.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the
 *         Plutus data was successfully set, or an appropriate error code if an error occurred, such as \ref CARDANO_ERROR_POINTER_IS_NULL
 *         if the \p redeemer is NULL.
 *
 * \note The function increases the reference count of the Plutus data object, so the caller retains ownership
 *       of the \p data reference and must manage its lifecycle. If the Plutus data is no longer needed after assignment,
 *       the caller should release their reference by calling \ref cardano_plutus_data_unref.
 *
 * Usage Example:
 * \code{.c}
 * cardano_redeemer_t* redeemer = ...; // Assume redeemer is already initialized
 * cardano_plutus_data_t* data = ...;  // Assume Plutus data is initialized
 *
 * cardano_error_t result = cardano_redeemer_set_data(redeemer, data);
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Plutus data was successfully set
 * }
 * else
 * {
 *   printf("Failed to set Plutus data: %s\n", cardano_error_to_string(result));
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_redeemer_set_data(cardano_redeemer_t* redeemer, cardano_plutus_data_t* data);

/**
 * \brief Retrieves the execution units associated with a redeemer.
 *
 * This function retrieves the \ref cardano_ex_units_t object from the given \ref cardano_redeemer_t.
 * The execution units (ExUnits) represent the computational cost required for the execution of the
 * Plutus script associated with the redeemer, measured in terms of memory and CPU units.
 *
 * \param[in] redeemer A constant pointer to an initialized \ref cardano_redeemer_t object from which the ExUnits will be retrieved.
 *
 * \return A pointer to the \ref cardano_ex_units_t object representing the execution units associated with the redeemer.
 *         The returned ExUnits is a new reference, and the caller is responsible for managing the lifecycle of this object.
 *         Specifically, the caller must release the ExUnits when it is no longer needed by calling \ref cardano_ex_units_unref.
 *         If the \p redeemer is NULL, the function will return NULL.
 *
 * Usage Example:
 * \code{.c}
 * const cardano_redeemer_t* redeemer = ...; // Assume redeemer is already initialized
 * cardano_ex_units_t* ex_units = cardano_redeemer_get_ex_units(redeemer);
 *
 * if (ex_units != NULL)
 * {
 *   // Use the execution units
 *
 *   // Ensure to clean up and release the ExUnits after use
 *   cardano_ex_units_unref(&ex_units);
 * }
 * else
 * {
 *   printf("Failed to retrieve the execution units.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_ex_units_t* cardano_redeemer_get_ex_units(cardano_redeemer_t* redeemer);

/**
 * \brief Sets the execution units for a redeemer.
 *
 * This function assigns the specified \ref cardano_ex_units_t object to the given \ref cardano_redeemer_t.
 * The execution units (ExUnits) represent the computational cost, in terms of memory and CPU, required
 * for the execution of the Plutus script associated with the redeemer.
 *
 * \param[in,out] redeemer A pointer to an initialized \ref cardano_redeemer_t object to which the ExUnits will be set.
 * \param[in] ex_units A pointer to an initialized \ref cardano_ex_units_t object representing the execution units to be assigned.
 *                     This parameter must not be NULL.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the ExUnits
 *         were successfully set, or an appropriate error code indicating the failure reason, such as \ref CARDANO_ERROR_POINTER_IS_NULL
 *         if either the \p redeemer or \p ex_units pointers are NULL.
 *
 * \note This function increases the reference count of the \p ex_units object. The caller retains ownership of their respective
 *       references and is responsible for releasing their reference to the \p ex_units when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_redeemer_t* redeemer = ...; // Assume redeemer is already initialized
 * cardano_ex_units_t* ex_units = cardano_ex_units_new(...); // Assume ex_units is already initialized
 *
 * cardano_error_t result = cardano_redeemer_set_ex_units(redeemer, ex_units);
 * if (result == CARDANO_SUCCESS)
 * {
 *   // The execution units have been set for the redeemer
 * }
 * else
 * {
 *   printf("Failed to set the execution units.\n");
 * }
 *
 * // Cleanup ex_units if no longer needed
 * cardano_ex_units_unref(&ex_units);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_redeemer_set_ex_units(cardano_redeemer_t* redeemer, cardano_ex_units_t* ex_units);

/**
 * \brief Clears the cached CBOR representation from a redeemer.
 *
 * This function removes the internally cached CBOR data from a \ref cardano_redeemer_t object.
 * It is useful when you have modified the redeemer after it was created from CBOR using
 * \ref cardano_redeemer_from_cbor and you want to ensure that the next serialization reflects
 * the current state of the redeemer, rather than using the original cached CBOR.
 *
 * \param[in,out] redeemer A pointer to an initialized \ref cardano_redeemer_t object
 *                         from which the CBOR cache will be cleared.
 *
 * \warning Clearing the CBOR cache may change the binary representation of the redeemer when
 *          serialized, which can alter the redeemer and invalidate any existing signatures.
 *          Use this function with caution, especially if the transaction has already been signed or
 *          if preserving the exact CBOR encoding is important for your application.
 *
 * Usage Example:
 * \code{.c}
 * // Assume redeemer was created using cardano_redeemer_from_cbor
 * cardano_redeemer_t* redeemer = ...;
 *
 * // Modify the redeemer as needed
 *
 * // Clear the CBOR cache to ensure serialization uses the updated redeemer
 * cardano_redeemer_clear_cbor_cache(redeemer);
 *
 * // Serialize the redeemer to CBOR
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * result = cardano_redeemer_to_cbor(redeemer, writer);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Process the CBOR data as needed
 * }
 * else
 * {
 *   const char* error_message = cardano_cbor_writer_get_last_error(writer);
 *   printf("Serialization failed: %s\n", error_message);
 * }
 *
 * // Clean up resources
 * cardano_cbor_writer_unref(&writer);
 * cardano_redeemer_unref(&redeemer);
 * \endcode
 */
CARDANO_EXPORT void cardano_redeemer_clear_cbor_cache(cardano_redeemer_t* redeemer);

/**
 * \brief Decrements the reference count of a cardano_redeemer_t object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_redeemer_t object
 * by decreasing its reference count. When the reference count reaches zero, the redeemer is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] redeemer A pointer to the pointer of the redeemer object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_redeemer_t* redeemer = cardano_redeemer_new(major, minor);
 *
 * // Perform operations with the redeemer...
 *
 * cardano_redeemer_unref(&redeemer);
 * // At this point, redeemer is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_redeemer_unref, the pointer to the \ref cardano_redeemer_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_redeemer_unref(cardano_redeemer_t** redeemer);

/**
 * \brief Increases the reference count of the cardano_redeemer_t object.
 *
 * This function is used to manually increment the reference count of an cardano_redeemer_t
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_redeemer_unref.
 *
 * \param redeemer A pointer to the cardano_redeemer_t object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming redeemer is a previously created redeemer object
 *
 * cardano_redeemer_ref(redeemer);
 *
 * // Now redeemer can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_redeemer_ref there is a corresponding
 * call to \ref cardano_redeemer_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_redeemer_ref(cardano_redeemer_t* redeemer);

/**
 * \brief Retrieves the current reference count of the cardano_redeemer_t object.
 *
 * This function returns the number of active references to an cardano_redeemer_t object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_redeemer_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param redeemer A pointer to the cardano_redeemer_t object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified cardano_redeemer_t object. If the object
 * is properly managed (i.e., every \ref cardano_redeemer_ref call is matched with a
 * \ref cardano_redeemer_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming redeemer is a previously created redeemer object
 *
 * size_t ref_count = cardano_redeemer_refcount(redeemer);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_redeemer_refcount(const cardano_redeemer_t* redeemer);

/**
 * \brief Sets the last error message for a given cardano_redeemer_t object.
 *
 * Records an error message in the redeemer's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] redeemer A pointer to the \ref cardano_redeemer_t instance whose last error message is
 *                       to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the redeemer's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_redeemer_set_last_error(
  cardano_redeemer_t* redeemer,
  const char*         message);

/**
 * \brief Retrieves the last error message recorded for a specific redeemer.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_redeemer_set_last_error for the given
 * redeemer. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] redeemer A pointer to the \ref cardano_redeemer_t instance whose last error
 *                   message is to be retrieved. If the redeemer is NULL, the function
 *                   returns a generic error message indicating the null redeemer.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified redeemer. If the redeemer is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_redeemer_set_last_error for the same redeemer, or until
 *       the redeemer is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_redeemer_get_last_error(
  const cardano_redeemer_t* redeemer);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_REDEEMER_H