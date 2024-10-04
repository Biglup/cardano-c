/**
 * \file committee.h
 *
 * \author angel.castillo
 * \date   Aug 14, 2024
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

#ifndef COMMITTEE_H
#define COMMITTEE_H

/* INCLUDES ******************************************************************/

#include <cardano/cbor/cbor_reader.h>
#include <cardano/cbor/cbor_writer.h>
#include <cardano/common/credential.h>
#include <cardano/common/unit_interval.h>
#include <cardano/error.h>
#include <cardano/export.h>
#include <cardano/proposal_procedures/credential_set.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief The constitutional committee represents a set of individuals or entities (each associated with a pair of Ed25519 credentials)
 * that are collectively responsible for ensuring that the Constitution is respected.
 *
 * Though it cannot be enforced on-chain, the constitutional committee is only supposed to vote on the constitutionality
 * of governance actions (which should thus ensure the long-term sustainability of the blockchain) and should be replaced
 * (via the no confidence action) if they overstep this boundary.
 */
typedef struct cardano_committee_t cardano_committee_t;

/**
 * \brief Creates and initializes a new instance of a constitutional committee.
 *
 * This function allocates and initializes a new instance of a \ref cardano_committee_t object,
 * which represents a constitutional committee responsible for overseeing the constitutionality of governance actions.
 * The function requires a quorum threshold, represented as a \ref cardano_unit_interval_t object, defining the minimum
 * percentage of committee members that must participate in a vote for it to be valid.
 *
 * \param[in] quorum_threshold A pointer to a \ref cardano_unit_interval_t object specifying the minimum percentage of committee members
 *            required to participate in a vote for it to be valid.
 * \param[out] committee On successful initialization, this will point to a newly created
 *             \ref cardano_committee_t object. This object represents a "strong reference"
 *             to the committee, meaning that it is fully initialized and ready for use.
 *             The caller is responsible for managing the lifecycle of this object,
 *             specifically, once the committee is no longer needed, the caller must release it
 *             by calling \ref cardano_committee_unref.
 *
 * \return \c cardano_error_t indicating the outcome of the operation. Returns \c CARDANO_SUCCESS if the committee was
 *         successfully created, or an appropriate error code indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_unit_interval_t* quorum_threshold = ...; // Assume quorum_threshold is already initialized
 * cardano_committee_t* committee = NULL;
 * cardano_error_t result = cardano_committee_new(&quorum_threshold, &committee);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the committee
 *   // Free resources when done
 *   cardano_committee_unref(&committee);
 * }
 * else
 * {
 *   printf("Failed to create the committee: %s\n", cardano_error_to_string(result));
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_committee_new(
  cardano_unit_interval_t* quorum_threshold,
  cardano_committee_t**    committee);

/**
 * \brief Creates a \ref cardano_committee_t from a CBOR reader.
 *
 * This function parses CBOR data using a provided \ref cardano_cbor_reader_t and constructs a \ref cardano_committee_t object.
 * It assumes that the CBOR reader is set up correctly and that the CBOR data corresponds to the structure expected for a committee.
 *
 * \param[in] reader A pointer to an initialized \ref cardano_cbor_reader_t that is ready to read the CBOR-encoded data.
 * \param[out] committee A pointer to a pointer of \ref cardano_committee_t that will be set to the address
 *                        of the newly created committee object upon successful decoding.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the object was successfully created, or an appropriate error code if an error occurred.
 *
 * \note If the function fails, the last error can be retrieved by calling \ref cardano_cbor_reader_get_last_error with the reader.
 *       The caller is responsible for freeing the created \ref cardano_committee_t object by calling
 *       \ref cardano_committee_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_new(cbor_data, data_size);
 * cardano_committee_t* committee = NULL;
 *
 * cardano_error_t result = cardano_committee_from_cbor(reader, &committee);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the committee
 *
 *   // Once done, ensure to clean up and release the committee
 *   cardano_committee_unref(&committee);
 * }
 * else
 * {
 *   const char* error = cardano_cbor_reader_get_last_error(reader);
 *   printf("Failed to decode committee: %s\n", error);
 * }
 *
 * cardano_cbor_reader_unref(&reader); // Cleanup the CBOR reader
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_committee_from_cbor(cardano_cbor_reader_t* reader, cardano_committee_t** committee);

/**
 * \brief Serializes the certificate into CBOR format using a CBOR writer.
 *
 * This function serializes the given \ref cardano_committee_t object using a \ref cardano_cbor_writer_t.
 *
 * \param[in] committee A constant pointer to the \ref cardano_committee_t object that is to be serialized.
 * \param[out] writer A pointer to a \ref cardano_cbor_writer_t where the CBOR serialized data will be written.
 *                    The writer must already be initialized and ready to accept the data.
 *
 * \return Returns \ref CARDANO_SUCCESS if the serialization is successful. If the \p committee or \p writer
 *         is NULL, returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_committee_t* committee = ...;
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * if (writer)
 * {
 *   cardano_error_t result = cardano_committee_to_cbor(committee, writer);
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
 * cardano_committee_unref(&committee);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_committee_to_cbor(
  const cardano_committee_t* committee,
  cardano_cbor_writer_t*     writer);

/**
 * \brief Sets the quorum threshold in the committee.
 *
 * This function updates the quorum threshold of a \ref cardano_committee_t object.
 * The quorum threshold is a \ref cardano_unit_interval_t object representing the minimum percentage of committee
 * members that must participate for a vote to be valid.
 *
 * \param[in,out] committee A pointer to an initialized \ref cardano_committee_t object to which the quorum threshold will be set.
 * \param[in] quorum_threshold A pointer to an initialized \ref cardano_unit_interval_t object representing the new quorum threshold.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the quorum threshold was
 *         successfully set, or an appropriate error code indicating the failure reason, such as \ref CARDANO_ERROR_POINTER_IS_NULL if any of the
 *         input pointers are NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_committee_t* committee = ...; // Assume committee is already initialized
 * cardano_unit_interval_t quorum_threshold = { .numerator = 60, .denominator = 100 }; // 60% quorum
 *
 * cardano_error_t result = cardano_committee_set_quorum_threshold(committee, &quorum_threshold);
 * if (result == CARDANO_SUCCESS)
 * {
 *   // The quorum threshold is now set for the committee
 * }
 * else
 * {
 *   printf("Failed to set the quorum threshold.\n");
 * }
 * // The committee needs to be managed and eventually unreferenced by the caller
 * cardano_committee_unref(&committee);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_committee_set_quorum_threshold(cardano_committee_t* committee, cardano_unit_interval_t* quorum_threshold);

/**
 * \brief Gets the quorum threshold from a committee.
 *
 * This function retrieves the quorum threshold from a given \ref cardano_committee_t object. The quorum threshold
 * is represented as a \ref cardano_unit_interval_t object which details the minimum percentage of committee members
 * required to validate a vote.
 *
 * \param[in] committee A pointer to an initialized \ref cardano_committee_t object from which the quorum threshold is retrieved.
 *
 * \return A pointer to the retrieved \ref cardano_unit_interval_t object representing the quorum threshold.
 *         This will be a new reference, and the caller is responsible for releasing it with \ref cardano_unit_interval_unref
 *         when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_committee_t* committee = ...; // Assume initialized
 * cardano_unit_interval_t* quorum_threshold = cardano_committee_get_quorum_threshold(committee);
 *
 * if (quorum_threshold != NULL)
 * {
 *   printf("Quorum Threshold: %u/%u\n", quorum_threshold->numerator, quorum_threshold->denominator);
 *   // Use the quorum threshold data
 *
 *   // Once done, ensure to clean up and release the quorum threshold
 *   cardano_unit_interval_unref(&quorum_threshold);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_unit_interval_t*
cardano_committee_get_quorum_threshold(cardano_committee_t* committee);

/**
 * \brief Retrieves a set of credentials for all members of a committee.
 *
 * This function fetches all member credentials from a given \ref cardano_committee_t object.
 * It returns a set of credentials, each representing a committee member. The function allocates memory
 * for a \ref cardano_credential_set_t object, and the caller is responsible for releasing this resource
 * using \ref cardano_credential_set_unref when it is no longer needed.
 *
 * \param[in] committee A pointer to an initialized \ref cardano_committee_t object.
 * \param[out] credentials On successful execution, this will point to a newly created \ref cardano_credential_set_t object
 *                         containing the credentials of all committee members. If the committee has no members,
 *                         the returned list will be empty but not NULL.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the list was
 *         successfully retrieved, or an appropriate error code indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_committee_t* committee = ...; // Assume committee is already initialized
 * cardano_credential_set_t* credentials = NULL;
 *
 * cardano_error_t result = cardano_committee_members_keys(committee, &credentials);
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Process the set of credentials
 *   // ...
 *
 *   // Once done, ensure to clean up and release the list
 *   cardano_credential_set_unref(&credentials);
 * }
 * else
 * {
 *   printf("Failed to retrieve committee members' credentials: %s\n", cardano_error_to_string(result));
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_committee_members_keys(cardano_committee_t* committee, cardano_credential_set_t** credentials);

/**
 * \brief Adds a member to a committee.
 *
 * This function adds a new member to the specified \ref cardano_committee_t object using a given \ref cardano_credential_t object and epoch.
 * The function assumes ownership of the credential reference passed to it, which means that the caller should not unreference the credential
 * after calling this function unless it retains another reference for its own use.
 *
 * \param[in,out] committee A pointer to an initialized \ref cardano_committee_t object to which the member will be added.
 * \param[in] credential A pointer to an initialized \ref cardano_credential_t object representing the member's credential.
 *                       This function increments the reference count of the credential, ensuring it remains valid for the duration
 *                       of its association with the committee.
 * \param[in] epoch The epoch number from which the member's participation in the committee becomes effective.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the member was
 *         successfully added, or an appropriate error code indicating the failure reason, such as \ref CARDANO_ERROR_POINTER_IS_NULL if any of the
 *         input pointers are NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_committee_t* committee = ...; // Assume committee is already initialized
 * cardano_credential_t* credential = ...; // Assume credential is already initialized
 * uint64_t epoch = 250; // Example epoch number
 *
 * cardano_error_t result = cardano_committee_add_member(committee, credential, epoch);
 * if (result == CARDANO_SUCCESS)
 * {
 *   // The member has been successfully added to the committee
 * }
 * else
 * {
 *   printf("Failed to add member to the committee: %s\n", cardano_error_to_string(result));
 * }
 * // Clean up resources, if no longer used elsewhere
 * cardano_credential_unref(&credential);
 * cardano_committee_unref(&committee);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_committee_add_member(cardano_committee_t* committee, cardano_credential_t* credential, uint64_t epoch);

/**
 * \brief Retrieves the epoch at which the term of a specific committee member will end.
 *
 * This function fetches the epoch number indicating when a committee member's term will end based on their credential.
 * It searches the committee's records for the specified \ref cardano_credential_t object and returns the associated epoch.
 * If the credential is not found, or if the member has no specified end term, the function returns 0.
 *
 * \param[in] committee A pointer to an initialized \ref cardano_committee_t object containing the committee data.
 * \param[in] credential A constant pointer to an initialized \ref cardano_credential_t object representing the committee member's credential.
 *
 * \return The epoch number at which the committee member's term will end, or 0 if the member's credential is not found or no end term is set.
 *
 * Usage Example:
 * \code{.c}
 * cardano_committee_t* committee = ...; // Assume committee is already initialized
 * const cardano_credential_t* credential = ...; // Assume credential represents a committee member
 *
 * uint64_t ending_epoch = cardano_committee_get_member_epoch(committee, credential);
 * if (ending_epoch > 0)
 * {
 *   printf("Member's term will end at epoch: %llu\n", ending_epoch);
 * }
 * else
 * {
 *   printf("Credential not found or no end term set for the committee member.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT uint64_t
cardano_committee_get_member_epoch(cardano_committee_t* committee, cardano_credential_t* credential);

/**
 * \brief Retrieves the credential at a specific index from the committee.
 *
 * This function retrieves the credential at the specified index from the committee.
 *
 * \param[in] committee Pointer to the committee object.
 * \param[in] index The index of the credential to retrieve.
 * \param[out] credential On successful retrieval, this will point to the credential
 *                        at the specified index. The caller is responsible for managing the lifecycle
 *                        of this object. Specifically, once the credential is no longer needed,
 *                        the caller must release it by calling \ref cardano_credential_unref.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the credential was successfully retrieved, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_committee_t* committee = NULL;
 * cardano_credential_t* credential = NULL;
 * size_t index = 0; // Index of the credential to retrieve
 *
 * // Assume committee is initialized properly
 *
 * cardano_error_t result = cardano_committee_get_key_at(committee, index, &credential);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the credential
 *
 *   // Once done, ensure to clean up and release the credential
 *   cardano_credential_unref(&credential);
 * }
 * else
 * {
 *   // Handle the error
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_committee_get_key_at(
  const cardano_committee_t* committee,
  size_t                     index,
  cardano_credential_t**     credential);

/**
 * \brief Retrieves the committee member epoch at a specific index from the committee.
 *
 * This function retrieves the committee member epoch at the specified index from the committee.
 *
 * \param[in] committee Pointer to the committee object.
 * \param[in] index The index of the committee member epoch to retrieve.
 * \param[out] epoch On successful retrieval, this will point to the committee member epoch
 *                    at the specified index.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the committee member epoch was successfully retrieved, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_committee_t* committee = NULL;
 * uint64_t epoch = 0;
 * size_t index = 0; // Index of the committee member epoch to retrieve
 *
 * // Assume committee is initialized properly
 *
 * cardano_error_t result = cardano_committee_get_value_at(committee, index, &epoch);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the epoch
 * }
 * else
 * {
 *   // Handle the error
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_committee_get_value_at(
  const cardano_committee_t* committee,
  size_t                     index,
  uint64_t*                  epoch);

/**
 * \brief Retrieves the credential and committee member epoch at the specified index.
 *
 * This function retrieves the credential and committee member epoch from the proposed committee member epochs
 * at the specified index.
 *
 * \param[in]  committee    Pointer to the proposed committee member epochs object.
 * \param[in]  index             The index at which to retrieve the key-value pair.
 * \param[out] credential On successful retrieval, this will point to the credential at the specified index.
 *                            The caller is responsible for managing the lifecycle of this object and should release it using
 *                            \ref cardano_credential_unref when it is no longer needed.
 * \param[out] epoch On successful retrieval, this will point to the committee member epoch at the specified index.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the key-value pair was successfully retrieved, or an appropriate error code indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_committee_t* committee = NULL;
 * // Assume committee is initialized properly
 *
 * size_t index = 0;
 * cardano_credential_t* credential = NULL;
 * uint64_t epoch = 0;
 *
 * cardano_error_t result = cardano_committee_get_key_value_at(committee, index, &credential, &epoch);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   if (credential != NULL && epoch != NULL)
 *   {
 *     // Use the credential and committee member epoch
 *   }
 *   else
 *   {
 *     printf("Key-value pair not set at index %zu.\n", index);
 *   }
 * }
 * else
 * {
 *   // Handle error
 *   printf("Failed to get key-value pair at index %zu.\n", index);
 * }
 *
 * // Clean up
 * cardano_committee_unref(&committee);
 * cardano_credential_unref(&credential);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_committee_get_key_value_at(
  const cardano_committee_t* committee,
  size_t                     index,
  cardano_credential_t**     credential,
  uint64_t*                  epoch);

/**
 * \brief Decrements the reference count of a cardano_committee_t object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_committee_t object
 * by decreasing its reference count. When the reference count reaches zero, the committee is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] committee A pointer to the pointer of the committee object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_committee_t* committee = cardano_committee_new(major, minor);
 *
 * // Perform operations with the committee...
 *
 * cardano_committee_unref(&committee);
 * // At this point, committee is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_committee_unref, the pointer to the \ref cardano_committee_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_committee_unref(cardano_committee_t** committee);

/**
 * \brief Increases the reference count of the cardano_committee_t object.
 *
 * This function is used to manually increment the reference count of an cardano_committee_t
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_committee_unref.
 *
 * \param committee A pointer to the cardano_committee_t object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming committee is a previously created committee object
 *
 * cardano_committee_ref(committee);
 *
 * // Now committee can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_committee_ref there is a corresponding
 * call to \ref cardano_committee_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_committee_ref(cardano_committee_t* committee);

/**
 * \brief Retrieves the current reference count of the cardano_committee_t object.
 *
 * This function returns the number of active references to an cardano_committee_t object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_committee_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param committee A pointer to the cardano_committee_t object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified cardano_committee_t object. If the object
 * is properly managed (i.e., every \ref cardano_committee_ref call is matched with a
 * \ref cardano_committee_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming committee is a previously created committee object
 *
 * size_t ref_count = cardano_committee_refcount(committee);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_committee_refcount(const cardano_committee_t* committee);

/**
 * \brief Sets the last error message for a given cardano_committee_t object.
 *
 * Records an error message in the committee's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] committee A pointer to the \ref cardano_committee_t instance whose last error message is
 *                       to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the committee's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_committee_set_last_error(
  cardano_committee_t* committee,
  const char*          message);

/**
 * \brief Retrieves the last error message recorded for a specific committee.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_committee_set_last_error for the given
 * committee. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] committee A pointer to the \ref cardano_committee_t instance whose last error
 *                   message is to be retrieved. If the committee is NULL, the function
 *                   returns a generic error message indicating the null committee.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified committee. If the committee is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_committee_set_last_error for the same committee, or until
 *       the committee is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_committee_get_last_error(
  const cardano_committee_t* committee);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // COMMITTEE_H