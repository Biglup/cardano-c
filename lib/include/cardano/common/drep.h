/**
 * \file drep.h
 *
 * \author angel.castillo
 * \date   Jun 18, 2024
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_DREP_H
#define BIGLUP_LABS_INCLUDE_CARDANO_DREP_H

/* INCLUDES ******************************************************************/

#include <cardano/cbor/cbor_reader.h>
#include <cardano/cbor/cbor_writer.h>
#include <cardano/common/credential.h>
#include <cardano/common/drep_type.h>
#include <cardano/error.h>
#include <cardano/export.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief In Voltaire, existing stake credentials will be able to delegate their stake to DReps
 * for voting purposes, in addition to the current delegation to stake pools for block production.
 *
 * Just as the number of blocks that a pool mint depends on the total stake, the amount of decision-making
 * power will depend on the number of coins delegated to a DRep.
 *
 * Registered DReps are identified by a credential that can be either:
 *
 * - A verification key (Ed25519)
 * - A native or Plutus script
 */
typedef struct cardano_drep_t cardano_drep_t;

/**
 * \brief Creates and initializes a new instance of a drep.
 *
 * This function allocates and initializes a new instance of \ref cardano_drep_t,
 * using the provided type and credential. It returns an error code to indicate the
 * success or failure of the operation.
 *
 * \param[in] type The type of the drep, represented by \ref cardano_drep_type_t.
 * \param[in] credential A pointer to \ref cardano_credential_t representing the credential
 *                       associated with this drep. The credential must be properly initialized
 *                       before being passed to this function. Must be NULL if \p type is
 *                       \ref CARDANO_DREP_TYPE_ABSTAIN or \ref CARDANO_DREP_TYPE_NO_CONFIDENCE.
 * \param[out] drep On successful initialization, this will point to a newly created
 *                  \ref cardano_drep_t object. This object represents a "strong reference"
 *                  to the drep, meaning that it is fully initialized and ready for use.
 *                  The caller is responsible for managing the lifecycle of this object.
 *                  Specifically, once the drep is no longer needed, the caller must release it
 *                  by calling \ref cardano_drep_unref.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the drep was successfully created, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_drep_type_t type = ...;  // Assume type is initialized here
 * cardano_credential_t* credential = ...;  // Assume credential is initialized here
 * cardano_drep_t* drep = NULL;
 *
 * // Attempt to create a new drep
 * cardano_error_t result = cardano_drep_new(type, credential, &drep);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the drep
 *
 *   // Once done, ensure to clean up and release the drep
 *   cardano_drep_unref(&drep);
 * }
 *
 * cardano_credential_unref(&credential);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_drep_new(
  cardano_drep_type_t   type,
  cardano_credential_t* credential,
  cardano_drep_t**      drep);

/**
 * \brief Converts a Bech32-encoded string representation of a DRep (Delegated Representative)
 *        into a `cardano_drep_t` object.
 *
 * The input string can follow one of two formats:
 *
 * 1. CIP-105 Format (DEPRECATED):
 *    - This format represents the key hash directly as a Bech32-encoded string.
 *
 * 2. CIP-129 Format:
 *    - This format introduces a header byte to encode additional metadata about the governance key type and credential type.
 *    - The binary structure is as follows:
 *
 *      Header Byte Structure:
 *      - The header byte consists of two parts:
 *        - Bits [7;4]: Key type (t t t t)
 *          - Defines the type of governance key being used.
 *          - Possible key types:
 *            - 0000 (CC Hot): Constitutional Committee Hot Key
 *            - 0001 (CC Cold): Constitutional Committee Cold Key
 *            - 0010 (DRep): Delegated Representative Key
 *        - Bits [3;0]: Credential type (c c c c)
 *          - Refers to the type of credential associated with the governance key.
 *          - Reserved values ensure no conflicts with Cardano address network tags:
 *            - 0010 (Key Hash): Key hash credential
 *            - 0011 (Script Hash): Script hash credential
 *
 * \param[in]  bech32_string   Pointer to the Bech32-encoded string.
 * \param[in]  string_length   Length of the input string.
 * \param[out] drep            Pointer to the output cardano_drep_t object.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the drep was successfully created, or an appropriate error code
 *         indicating the failure reason.
 *
 * \note The caller is responsible for freeing the memory associated with the returned
 *       cardano_drep_t object using `cardano_drep_unref`.
 *
 * \code{.c}
 * const char* drep_bech32 = "drep1qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq";
 * size_t drep_length = strlen(drep_bech32);
 * cardano_drep_t* drep = NULL;
 *
 * cardano_error_t result = cardano_drep_from_string(drep_bech32, drep_length, &drep);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("DRep object successfully created.\n");
 *   // Use the drep object...
 *   cardano_drep_unref(drep);
 * }
 * else
 * {
 *   printf("Failed to parse DRep string: %d\n", result);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_drep_from_string(
  const char*      bech32_string,
  size_t           string_length,
  cardano_drep_t** drep);

/**
 * \brief Creates a drep from a CBOR reader.
 *
 * This function parses CBOR data using a provided \ref cardano_cbor_reader_t and constructs a \ref cardano_drep_t object.
 * It assumes that the CBOR reader is set up correctly and that the CBOR data corresponds to the structure expected for a drep.
 *
 * \param[in] reader A pointer to an initialized \ref cardano_cbor_reader_t that is ready to read the CBOR-encoded drep data.
 * \param[out] drep A pointer to a pointer of \ref cardano_drep_t that will be set to the address
 *                        of the newly created drep object upon successful decoding.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the drep was successfully created, or an appropriate error code if an error occurred.
 *
 * \note If the function fails, the last error can be retrieved by calling \ref cardano_cbor_reader_get_last_error with the reader.
 *       The caller is responsible for freeing the created \ref cardano_drep_t object by calling
 *       \ref cardano_drep_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_new(cbor_data, data_size);
 * cardano_drep_t* drep = NULL;
 *
 * cardano_error_t result = cardano_drep_from_cbor(reader, &drep);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the drep
 *
 *   // Once done, ensure to clean up and release the drep
 *   cardano_drep_unref(&drep);
 * }
 * else
 * {
 *   const char* error = cardano_cbor_reader_get_last_error(reader);
 *   printf("Failed to decode drep: %s\n", error);
 * }
 *
 * cardano_cbor_reader_unref(&reader); // Cleanup the CBOR reader
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_drep_from_cbor(cardano_cbor_reader_t* reader, cardano_drep_t** drep);

/**
 * \brief Serializes a drep into CBOR format using a CBOR writer.
 *
 * This function serializes the given \ref cardano_drep_t object using a \ref cardano_cbor_writer_t.
 *
 * \param[in] drep A constant pointer to the \ref cardano_drep_t object that is to be serialized.
 * \param[out] writer A pointer to a \ref cardano_cbor_writer_t where the CBOR serialized data will be written.
 *                    The writer must already be initialized and ready to accept the data.
 *
 * \return Returns \ref CARDANO_SUCCESS if the serialization is successful. If the \p drep or \p writer
 *         is NULL, returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_drep_t* drep = ...;
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * if (writer)
 * {
 *   cardano_error_t result = cardano_drep_to_cbor(drep, writer);
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
 * cardano_drep_unref(&drep);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_drep_to_cbor(
  const cardano_drep_t*  drep,
  cardano_cbor_writer_t* writer);

/**
 * \brief Retrieves the size needed for the string representation (CIP-129) of a Cardano DRep.
 *
 * This function calculates the size of the buffer required to hold the string representation
 * of a \ref cardano_drep_t object, including the null terminator. This size is necessary
 * to ensure that the buffer allocated for converting the DRep to a string is sufficient.
 *
 * \param[in] drep A constant pointer to the \ref cardano_drep_t object for which the string
 *                 size is being calculated. The object must not be NULL.
 *
 * \return The size in bytes needed to store the string representation of the DRep, including
 *         the null terminator. If the input \p DRep is NULL, the behavior is undefined.
 *
 * Usage Example:
 * \code{.c}
 * cardano_drep_t* drep = cardano_drep_new(...);
 * size_t required_size = cardano_drep_get_string_size(drep);
 *
 * char* drep_str = (char*)malloc(required_size);
 *
 * if (drep_str)
 * {
 *   cardano_error_t result = cardano_drep_to_string(drep, drep_str, required_size);
 *   if (result == CARDANO_SUCCESS)
 *   {
 *     printf("DRep: %s\n", drep_str);
 *   }
 *   free(drep_str);
 * }
 *
 * // Clean up the drep object once done
 * cardano_drep_unref(&drep);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_drep_get_string_size(const cardano_drep_t* drep);

/**
 * \brief Converts a Cardano DRep into its string representation (CIP-129).
 *
 * This function serializes the given \ref cardano_drep_t object into a string format.
 * The string is written to a user-provided buffer, and the size of this buffer must be
 * adequate to hold the entire string, including the null terminator. The required size
 * can be determined by calling \ref cardano_drep_get_string_size.
 *
 * \param[in] drep A constant pointer to the \ref cardano_drep_t object that is to be converted to a string.
 *                 The object must not be NULL.
 * \param[out] data A pointer to the buffer where the string representation of the drep will be written.
 * \param[in] size The size of the buffer pointed to by \p data. This size should be at least as large as the value
 *                 returned by \ref cardano_drep_get_string_size to ensure successful serialization.
 *
 * \return Returns \ref CARDANO_SUCCESS if the conversion is successful. If the buffer is too small, returns
 *         \ref CARDANO_ERROR_INSUFFICIENT_BUFFER_SIZE. If the \p drep or \p data is NULL, returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_drep_t* drep = cardano_drep_new(...);
 * size_t required_size = cardano_drep_get_string_size(drep);
 * char* drep_str = (char*)malloc(required_size);
 *
 * if (drep_str)
 * {
 *   cardano_error_t result = cardano_drep_to_string(drep, drep_str, required_size);
 *   if (result == CARDANO_SUCCESS)
 *   {
 *     printf("Address: %s\n", drep_str);
 *   }
 *
 *   free(drep_str);
 * }
 *
 * // Clean up the drep object once done
 * cardano_drep_unref(&drep);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_drep_to_string(
  const cardano_drep_t* drep,
  char*                 data,
  size_t                size);

/**
 * \brief Retrieves the credential associated with a drep.
 *
 * This function retrieves the credential associated with the given \ref cardano_drep_t object.
 * The retrieved credential is returned via the \p credential output parameter.
 *
 * \param[in] drep A pointer to the \ref cardano_drep_t object from which to retrieve the credential.
 * \param[out] credential A pointer to a pointer that will be set to the address of the retrieved
 *                        \ref cardano_credential_t object. The caller is responsible for managing
 *                        the lifecycle of this credential. Specifically, once the credential is no
 *                        longer needed, the caller must release it by calling \ref cardano_credential_unref.
 *                        Will be NULL if the \p drep is of type \ref CARDANO_DREP_TYPE_ABSTAIN or
 *                        \ref CARDANO_DREP_TYPE_NO_CONFIDENCE.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the credential was successfully retrieved, or an appropriate error code indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_drep_t* drep = ...;  // Assume drep is initialized here
 * cardano_credential_t* credential = NULL;
 *
 * // Retrieve the credential
 * cardano_error_t result = cardano_drep_get_credential(drep, &credential);
 *
 * if (result == CARDANO_SUCCESS && credential != NULL)
 * {
 *   // Use the credential
 *
 *   // Once done, ensure to clean up and release the credential
 *   cardano_credential_unref(&credential);
 * }
 *
 * cardano_drep_unref(&drep);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_drep_get_credential(cardano_drep_t* drep, cardano_credential_t** credential);

/**
 * \brief Sets the credential associated with a drep.
 *
 * This function sets the credential for the given \ref cardano_drep_t object.
 *
 * \param[in,out] drep A pointer to the \ref cardano_drep_t object for which to set the credential.
 * \param[in] credential A pointer to the \ref cardano_credential_t object to be associated with the drep.
 *                       The \p credential must be properly initialized before being passed to this function.
 *                       For \ref CARDANO_DREP_TYPE_ABSTAIN and \ref CARDANO_DREP_TYPE_NO_CONFIDENCE, this parameter
 *                       must be NULL.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the credential was successfully set, or an appropriate error code indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_drep_t* drep = ...;  // Assume drep is initialized here
 * cardano_credential_t* credential = ...;  // Assume credential is initialized here
 *
 * // Set the credential
 * cardano_error_t result = cardano_drep_set_credential(drep, credential);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Credential was successfully set
 * }
 *
 * // Clean up
 * cardano_drep_unref(&drep);
 * cardano_credential_unref(&credential);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_drep_set_credential(cardano_drep_t* drep, cardano_credential_t* credential);

/**
 * \brief Retrieves the type of a drep.
 *
 * This function gets the type of the given \ref cardano_drep_t object.
 *
 * \param[in] drep A constant pointer to the \ref cardano_drep_t object.
 * \param[out] type A pointer to a \ref cardano_drep_type_t variable where the type will be stored.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the type was successfully retrieved, or an appropriate error code indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_drep_t* drep = ...;  // Assume drep is initialized here
 * cardano_drep_type_t type;
 *
 * // Get the drep type
 * cardano_error_t result = cardano_drep_get_type(drep, &type);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // The type was successfully retrieved
 * }
 *
 * // Clean up
 * cardano_drep_unref(&drep);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_drep_get_type(const cardano_drep_t* drep, cardano_drep_type_t* type);

/**
 * \brief Sets the type of a drep.
 *
 * This function sets the type of the given \ref cardano_drep_t object.
 *
 * \param[in,out] drep A pointer to the \ref cardano_drep_t object whose type is to be set.
 * \param[in] type The \ref cardano_drep_type_t value to set for the drep.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the type was successfully set, or an appropriate error code indicating the failure reason.
 *
 * \remark If the type is set to \ref CARDANO_DREP_TYPE_ABSTAIN or \ref CARDANO_DREP_TYPE_NO_CONFIDENCE,
 *         the credential must be NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_drep_t* drep = ...;  // Assume drep is initialized here
 * cardano_drep_type_t type = CARDANO_DREP_TYPE_KEY_HASH;
 *
 * // Set the drep type
 * cardano_error_t result = cardano_drep_set_type(drep, type);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // The type was successfully set
 * }
 *
 * // Clean up
 * cardano_drep_unref(&drep);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_drep_set_type(cardano_drep_t* drep, cardano_drep_type_t type);

/**
 * \brief Decrements the reference count of a drep object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_drep_t object
 * by decreasing its reference count. When the reference count reaches zero, the drep is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] drep A pointer to the pointer of the drep object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_drep_t* drep = cardano_drep_new();
 *
 * // Perform operations with the drep...
 *
 * cardano_drep_unref(&drep);
 * // At this point, drep is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_drep_unref, the pointer to the \ref cardano_drep_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_drep_unref(cardano_drep_t** drep);

/**
 * \brief Increases the reference count of the cardano_drep_t object.
 *
 * This function is used to manually increment the reference count of a drep
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_drep_unref.
 *
 * \param drep A pointer to the drep object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming drep is a previously created drep object
 *
 * cardano_drep_ref(drep);
 *
 * // Now drep can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_drep_ref there is a corresponding
 * call to \ref cardano_drep_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_drep_ref(cardano_drep_t* drep);

/**
 * \brief Retrieves the current reference count of the cardano_drep_t object.
 *
 * This function returns the number of active references to a drep object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_drep_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param drep A pointer to the drep object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified drep object. If the object
 * is properly managed (i.e., every \ref cardano_drep_ref call is matched with a
 * \ref cardano_drep_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming drep is a previously created drep object
 *
 * size_t ref_count = cardano_drep_refcount(drep);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_drep_refcount(const cardano_drep_t* drep);

/**
 * \brief Sets the last error message for a given drep object.
 *
 * Records an error message in the drep's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] drep A pointer to the \ref cardano_drep_t instance whose last error message is
 *                       to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the drep's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_drep_set_last_error(cardano_drep_t* drep, const char* message);

/**
 * \brief Retrieves the last error message recorded for a specific drep.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_drep_set_last_error for the given
 * drep. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] drep A pointer to the \ref cardano_drep_t instance whose last error
 *                   message is to be retrieved. If the drep is NULL, the function
 *                   returns a generic error message indicating the null drep.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified drep. If the drep is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_drep_set_last_error for the same drep, or until
 *       the drep is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_drep_get_last_error(const cardano_drep_t* drep);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_DREP_H