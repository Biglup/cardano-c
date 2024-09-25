/**
 * \file voting_procedure.h
 *
 * \author angel.castillo
 * \date   Aug 08, 2024
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_VOTING_PROCEDURE_H
#define BIGLUP_LABS_INCLUDE_CARDANO_VOTING_PROCEDURE_H

/* INCLUDES ******************************************************************/

#include <cardano/cbor/cbor_reader.h>
#include <cardano/cbor/cbor_writer.h>
#include <cardano/common/anchor.h>
#include <cardano/error.h>
#include <cardano/export.h>
#include <cardano/voting_procedures/vote.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief A voting procedure is a pair of:
 *
 * - a vote.
 * - an anchor, it links the vote to arbitrary off-chain JSON payload of metadata.
 */
typedef struct cardano_voting_procedure_t cardano_voting_procedure_t;

/**
 * \brief Creates and initializes a new voting procedure.
 *
 * This function allocates and initializes a new instance of a \ref cardano_voting_procedure_t object
 * using the provided \ref cardano_voter_t and \ref cardano_anchor_t objects.
 *
 * \param[in] vote A pointer to the \ref cardano_voter_t object that represents the voter initiating the procedure.
 * \param[in] anchor A pointer to the \ref cardano_anchor_t object that defines the starting point or condition for the voting procedure.
 * \param[out] voting_procedure On successful initialization, this will point to a newly created
 *             \ref cardano_voting_procedure_t object. This object represents a "strong reference"
 *             to the voting procedure, meaning that it is fully initialized and ready for use.
 *             The caller is responsible for managing the lifecycle of this object.
 *             Specifically, once the voting procedure is no longer needed, the caller must release it
 *             by calling \ref cardano_voting_procedure_unref.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the voting procedure was successfully created, or an appropriate error code
 *         indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_vote_t vote = CARDANO_VOTE_YES;
 * cardano_anchor_t* anchor = cardano_anchor_new(...);
 * cardano_voting_procedure_t* voting_procedure = NULL;
 *
 * cardano_error_t result = cardano_voting_procedure_new(vote, anchor, &voting_procedure);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the voting procedure
 *
 *   // Once done, ensure to clean up and release the voting procedure
 *   cardano_voting_procedure_unref(&voting_procedure);
 * }
 *
 * // Clean up other resources
 * cardano_anchor_unref(&anchor);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_voting_procedure_new(
  cardano_vote_t               vote,
  cardano_anchor_t*            anchor,
  cardano_voting_procedure_t** voting_procedure);

/**
 * \brief Creates a voting_procedure from a CBOR reader.
 *
 * This function parses CBOR data using a provided \ref cardano_cbor_reader_t and constructs a \ref cardano_voting_procedure_t object.
 * It assumes that the CBOR reader is set up correctly and that the CBOR data corresponds to the structure expected for a voting_procedure.
 *
 * \param[in] reader A pointer to an initialized \ref cardano_cbor_reader_t that is ready to read the CBOR-encoded voting_procedure data.
 * \param[out] voting_procedure A pointer to a pointer of \ref cardano_voting_procedure_t that will be set to the address
 *                        of the newly created voting_procedure object upon successful decoding.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the voting_procedure was successfully created, or an appropriate error code if an error occurred.
 *
 * \note If the function fails, the last error can be retrieved by calling \ref cardano_cbor_reader_get_last_error with the reader.
 *       The caller is responsible for freeing the created \ref cardano_voting_procedure_t object by calling
 *       \ref cardano_voting_procedure_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_new(cbor_data, data_size);
 * cardano_voting_procedure_t* voting_procedure = NULL;
 *
 * cardano_error_t result = cardano_voting_procedure_from_cbor(reader, &voting_procedure);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the voting_procedure
 *
 *   // Once done, ensure to clean up and release the voting_procedure
 *   cardano_voting_procedure_unref(&voting_procedure);
 * }
 * else
 * {
 *   const char* error = cardano_cbor_reader_get_last_error(reader);
 *   printf("Failed to decode voting_procedure: %s\n", error);
 * }
 *
 * cardano_cbor_reader_unref(&reader); // Cleanup the CBOR reader
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_voting_procedure_from_cbor(cardano_cbor_reader_t* reader, cardano_voting_procedure_t** voting_procedure);

/**
 * \brief Serializes a voting_procedure into CBOR format using a CBOR writer.
 *
 * This function serializes the given \ref cardano_voting_procedure_t object using a \ref cardano_cbor_writer_t.
 *
 * \param[in] voting_procedure A constant pointer to the \ref cardano_voting_procedure_t object that is to be serialized.
 * \param[out] writer A pointer to a \ref cardano_cbor_writer_t where the CBOR serialized data will be written.
 *                    The writer must already be initialized and ready to accept the data.
 *
 * \return Returns \ref CARDANO_SUCCESS if the serialization is successful. If the \p voting_procedure or \p writer
 *         is NULL, returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_voting_procedure_t* voting_procedure = ...;
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * if (writer)
 * {
 *   cardano_error_t result = cardano_voting_procedure_to_cbor(voting_procedure, writer);
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
 * cardano_voting_procedure_unref(&voting_procedure);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_voting_procedure_to_cbor(
  const cardano_voting_procedure_t* voting_procedure,
  cardano_cbor_writer_t*            writer);

/**
 * \brief Retrieves the vote type from a voting procedure.
 *
 * This function retrieves the vote type from the given \ref cardano_voting_procedure_t object.
 * The vote type is represented by the \ref cardano_vote_t enumeration which includes options such as
 * CARDANO_VOTE_NO, CARDANO_VOTE_YES, and CARDANO_VOTE_ABSTAIN.
 *
 * \param[in] voting_procedure A constant pointer to the \ref cardano_voting_procedure_t object
 *                             from which the vote type is to be retrieved.
 *
 * \return The vote type as \ref cardano_vote_t. If the input is NULL, the behavior is undefined.
 *
 * Usage Example:
 * \code{.c}
 * cardano_voting_procedure_t* voting_procedure = ...; // Assume voting_procedure is already initialized
 * cardano_vote_t vote = cardano_voting_procedure_get_vote(voting_procedure);
 *
 * switch (vote) {
 *     case CARDANO_VOTE_YES:
 *         printf("The vote is YES.\n");
 *         break;
 *     case CARDANO_VOTE_NO:
 *         printf("The vote is NO.\n");
 *         break;
 *     case CARDANO_VOTE_ABSTAIN:
 *         printf("The vote is ABSTAIN.\n");
 *         break;
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_vote_t cardano_voting_procedure_get_vote(const cardano_voting_procedure_t* voting_procedure);

/**
 * \brief Sets the vote type in a voting procedure.
 *
 * This function sets the vote type for the specified \ref cardano_voting_procedure_t object.
 * The vote type is defined by the \ref cardano_vote_t enumeration, which includes options such as
 * \ref CARDANO_VOTE_NO, \ref CARDANO_VOTE_YES, and \ref CARDANO_VOTE_ABSTAIN.
 *
 * \param[in,out] voting_procedure A pointer to the \ref cardano_voting_procedure_t object
 *                                 in which the vote type will be set. This object must be previously
 *                                 initialized and cannot be NULL.
 * \param[in] vote The vote type as \ref cardano_vote_t to be set in the voting procedure.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the vote type was successfully set, or an appropriate error code indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_voting_procedure_t* voting_procedure = ...; // Assume voting_procedure is already initialized
 * cardano_error_t result = cardano_voting_procedure_set_vote(voting_procedure, CARDANO_VOTE_YES);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Vote set successfully.\n");
 * }
 * else
 * {
 *   printf("Failed to set the vote.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_voting_procedure_set_vote(cardano_voting_procedure_t* voting_procedure, cardano_vote_t vote);

/**
 * \brief Retrieves the anchor from a voting procedure.
 *
 * This function returns the anchor associated with a given \ref cardano_voting_procedure_t object.
 * The anchor is an optional field and may not be set, in which case the function will return NULL.
 *
 * \param[in] voting_procedure A constant pointer to the \ref cardano_voting_procedure_t object
 *                             from which the anchor is to be retrieved. This object must be previously
 *                             initialized and cannot be NULL.
 *
 * \return A pointer to the \ref cardano_anchor_t object if it exists, otherwise NULL if the anchor
 *         is not set or if the input voting_procedure is NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_voting_procedure_t* voting_procedure = ...; // Assume voting_procedure is already initialized
 * cardano_anchor_t* anchor = cardano_voting_procedure_get_anchor(voting_procedure);
 *
 * if (anchor)
 * {
 *   printf("Anchor retrieved successfully.\n");
 * }
 * else
 * {
 *   printf("No anchor is set for this voting procedure.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_anchor_t* cardano_voting_procedure_get_anchor(cardano_voting_procedure_t* voting_procedure);

/**
 * \brief Sets or unsets the anchor in a voting procedure.
 *
 * This function assigns an anchor to a \ref cardano_voting_procedure_t object. The anchor can be
 * optionally unset by passing NULL as the anchor parameter.
 *
 * \param[in] voting_procedure A pointer to the \ref cardano_voting_procedure_t object to which
 *                             the anchor will be set. This object must be previously initialized
 *                             and cannot be NULL.
 * \param[in] anchor A pointer to the \ref cardano_anchor_t object representing the anchor to be
 *                   set. If this parameter is NULL, the anchor in the voting procedure will be
 *                   unset.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the anchor was successfully set or unset, or an appropriate error code indicating the
 *         failure reason, such as \ref CARDANO_ERROR_POINTER_IS_NULL if the voting_procedure pointer is NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_voting_procedure_t* voting_procedure = ...; // Assume voting_procedure is already initialized
 * cardano_anchor_t* anchor = ...; // Assume anchor is initialized or NULL to unset
 *
 * cardano_error_t result = cardano_voting_procedure_set_anchor(voting_procedure, anchor);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Anchor set/unset successfully.\n");
 * }
 * else
 * {
 *   printf("Failed to set/unset the anchor.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_voting_procedure_set_anchor(cardano_voting_procedure_t* voting_procedure, cardano_anchor_t* anchor);

/**
 * \brief Decrements the reference count of a voting_procedure object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_voting_procedure_t object
 * by decreasing its reference count. When the reference count reaches zero, the voting_procedure is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] voting_procedure A pointer to the pointer of the voting_procedure object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_voting_procedure_t* voting_procedure = cardano_voting_procedure_new(...);
 *
 * // Perform operations with the voting_procedure...
 *
 * cardano_voting_procedure_unref(&voting_procedure);
 * // At this point, voting_procedure is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_voting_procedure_unref, the pointer to the \ref cardano_voting_procedure_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_voting_procedure_unref(cardano_voting_procedure_t** voting_procedure);

/**
 * \brief Increases the reference count of the cardano_voting_procedure_t object.
 *
 * This function is used to manually increment the reference count of a voting_procedure
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_voting_procedure_unref.
 *
 * \param voting_procedure A pointer to the voting_procedure object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming voting_procedure is a previously created voting_procedure object
 *
 * cardano_voting_procedure_ref(voting_procedure);
 *
 * // Now voting_procedure can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_voting_procedure_ref there is a corresponding
 * call to \ref cardano_voting_procedure_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_voting_procedure_ref(cardano_voting_procedure_t* voting_procedure);

/**
 * \brief Retrieves the current reference count of the cardano_voting_procedure_t object.
 *
 * This function returns the number of active references to a voting_procedure object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_voting_procedure_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param voting_procedure A pointer to the voting_procedure object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified voting_procedure object. If the object
 * is properly managed (i.e., every \ref cardano_voting_procedure_ref call is matched with a
 * \ref cardano_voting_procedure_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming voting_procedure is a previously created voting_procedure object
 *
 * size_t ref_count = cardano_voting_procedure_refcount(voting_procedure);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_voting_procedure_refcount(const cardano_voting_procedure_t* voting_procedure);

/**
 * \brief Sets the last error message for a given voting_procedure object.
 *
 * Records an error message in the voting_procedure's last_error buffer, overwriting any existing message.
 * This is useful for storing devoting_procedureive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] voting_procedure A pointer to the \ref cardano_voting_procedure_t instance whose last error message is
 *                       to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the voting_procedure's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_voting_procedure_set_last_error(cardano_voting_procedure_t* voting_procedure, const char* message);

/**
 * \brief Retrieves the last error message recorded for a specific voting_procedure.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_voting_procedure_set_last_error for the given
 * voting_procedure. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] voting_procedure A pointer to the \ref cardano_voting_procedure_t instance whose last error
 *                   message is to be retrieved. If the voting_procedure is NULL, the function
 *                   returns a generic error message indicating the null voting_procedure.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified voting_procedure. If the voting_procedure is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_voting_procedure_set_last_error for the same voting_procedure, or until
 *       the voting_procedure is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_voting_procedure_get_last_error(const cardano_voting_procedure_t* voting_procedure);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_VOTING_PROCEDURE_H