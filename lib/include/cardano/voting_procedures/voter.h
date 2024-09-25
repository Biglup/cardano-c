/**
 * \file voter.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_VOTER_H
#define BIGLUP_LABS_INCLUDE_CARDANO_VOTER_H

/* INCLUDES ******************************************************************/

#include <cardano/cbor/cbor_reader.h>
#include <cardano/cbor/cbor_writer.h>
#include <cardano/common/credential.h>
#include <cardano/error.h>
#include <cardano/export.h>
#include <cardano/typedefs.h>
#include <cardano/voting_procedures/voter_type.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief A voter is any participant with an eligible role who either has a direct stake or has delegated their stake,
 * and they exercise their rights by casting votes on governance actions. The weight or influence of their vote
 * is determined by the amount of their active stake or the stake that's been delegated to them.
 *
 * Various roles in the Cardano ecosystem can participate in voting. This includes constitutional committee members,
 * DReps (Delegation Representatives), and SPOs (Stake Pool Operators).
 */
typedef struct cardano_voter_t cardano_voter_t;

/**
 * \brief Creates and initializes a new instance of a voter.
 *
 * This function creates and initializes a new instance of a \ref cardano_voter_t object based on the specified voter type and credentials.
 * It returns an error code to indicate the success or failure of the operation.
 *
 * \param[in] voter_type The type of the voter to create.
 * \param[in] credential A pointer to the \ref cardano_credential_t object containing the credentials of the voter. The object must not be NULL.
 * \param[out] voter On successful initialization, this will point to a newly created
 *             \ref cardano_voter_t object. This object represents a "strong reference"
 *             to the voter, meaning that it is fully initialized and ready for use.
 *             The caller is responsible for managing the lifecycle of this object.
 *             Specifically, once the voter is no longer needed, the caller must release it
 *             by calling \ref cardano_voter_unref.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the voter was successfully created, or an appropriate error code indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_credential_t* credential = cardano_credential_new(...);
 * cardano_voter_t* voter = NULL;
 *
 * // Attempt to create a new voter
 * cardano_error_t result = cardano_voter_new(CARDANO_VOTER_TYPE_CONSTITUTIONAL_COMMITTEE_KEY_HASH, credential, &voter);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the voter
 *
 *   // Once done, ensure to clean up and release the voter
 *   cardano_voter_unref(&voter);
 * }
 *
 * // Clean up the credential object once done
 * cardano_credential_unref(&credential);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_voter_new(
  cardano_voter_type_t  voter_type,
  cardano_credential_t* credential,
  cardano_voter_t**     voter);

/**
 * \brief Creates a voter from a CBOR reader.
 *
 * This function parses CBOR data using a provided \ref cardano_cbor_reader_t and constructs a \ref cardano_voter_t object.
 * It assumes that the CBOR reader is set up correctly and that the CBOR data corresponds to the structure expected for a voter.
 *
 * \param[in] reader A pointer to an initialized \ref cardano_cbor_reader_t that is ready to read the CBOR-encoded voter data.
 * \param[out] voter A pointer to a pointer of \ref cardano_voter_t that will be set to the address
 *                        of the newly created voter object upon successful decoding.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the voter was successfully created, or an appropriate error code if an error occurred.
 *
 * \note If the function fails, the last error can be retrieved by calling \ref cardano_cbor_reader_get_last_error with the reader.
 *       The caller is responsible for freeing the created \ref cardano_voter_t object by calling
 *       \ref cardano_voter_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_new(cbor_data, data_size);
 * cardano_voter_t* voter = NULL;
 *
 * cardano_error_t result = cardano_voter_from_cbor(reader, &voter);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the voter
 *
 *   // Once done, ensure to clean up and release the voter
 *   cardano_voter_unref(&voter);
 * }
 * else
 * {
 *   const char* error = cardano_cbor_reader_get_last_error(reader);
 *   printf("Failed to decode voter: %s\n", error);
 * }
 *
 * cardano_cbor_reader_unref(&reader); // Cleanup the CBOR reader
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_voter_from_cbor(cardano_cbor_reader_t* reader, cardano_voter_t** voter);

/**
 * \brief Serializes a voter into CBOR format using a CBOR writer.
 *
 * This function serializes the given \ref cardano_voter_t object using a \ref cardano_cbor_writer_t.
 *
 * \param[in] voter A constant pointer to the \ref cardano_voter_t object that is to be serialized.
 * \param[out] writer A pointer to a \ref cardano_cbor_writer_t where the CBOR serialized data will be written.
 *                    The writer must already be initialized and ready to accept the data.
 *
 * \return Returns \ref CARDANO_SUCCESS if the serialization is successful. If the \p voter or \p writer
 *         is NULL, returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_voter_t* voter = ...;
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * if (writer)
 * {
 *   cardano_error_t result = cardano_voter_to_cbor(voter, writer);
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
 * cardano_voter_unref(&voter);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_voter_to_cbor(
  const cardano_voter_t* voter,
  cardano_cbor_writer_t* writer);

/**
 * \brief Sets the type for a voter.
 *
 * This function assigns a type to the specified \ref cardano_voter_t object.
 *
 * \param[in,out] voter A pointer to the \ref cardano_voter_t object for which the type is to be set.
 * \param[in] type The voter type to set, as defined by \ref cardano_voter_type_t.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the voter type was successfully set, or an appropriate error code indicating the failure reason,
 *         such as \ref CARDANO_ERROR_POINTER_IS_NULL if any of the input pointers are NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_voter_t* voter = cardano_voter_new(...);
 * cardano_voter_type_t voter_type = CARDANO_VOTER_TYPE_DREP_KEY_HASH;
 *
 * cardano_error_t result = cardano_voter_set_type(voter, voter_type);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // The voter type has been set successfully
 * }
 * else
 * {
 *   printf("Failed to set the voter type.\n");
 * }
 *
 * // Clean up the voter object once done
 * cardano_voter_unref(&voter);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_voter_set_type(
  cardano_voter_t*     voter,
  cardano_voter_type_t type);

/**
 * \brief Retrieves the type of the voter.
 *
 * This function retrieves the type of a given \ref cardano_voter_t object and stores it in the provided
 * output parameter. The voter type is defined in the \ref cardano_voter_type_t enumeration.
 *
 * \param[in] voter A constant pointer to the \ref cardano_voter_t object from which
 *                        the type is to be retrieved. The object must not be NULL.
 * \param[out] type Pointer to a variable where the voter type will be stored. This variable will
 *                      be set to the value from the \ref cardano_voter_type_t enumeration.
 *
 * \return \ref CARDANO_SUCCESS if the type was successfully retrieved, or an appropriate error code if the
 *         input is NULL or any other error occurs.
 *
 * Usage Example:
 * \code{.c}
 * cardano_voter_t* voter = cardano_voter_new(...);
 * cardano_voter_type_t type;
 * cardano_error_t result = cardano_voter_get_type(voter, &type);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   switch(type)
 *   {
 *     case CARDANO_VOTER_TYPE_CONSTITUTIONAL_COMMITTEE_KEY_HASH:
 *       printf("Voter is a constitutional committee member identified by key hash.\n");
 *       break;
 *     case CARDANO_VOTER_TYPE_CONSTITUTIONAL_COMMITTEE_SCRIPT_HASH:
 *       printf("Voter is a constitutional committee member identified by script hash.\n");
 *       break;
 *     case CARDANO_VOTER_TYPE_DREP_KEY_HASH:
 *       printf("Voter is a DRep identified by key hash.\n");
 *       break;
 *     case CARDANO_VOTER_TYPE_DREP_SCRIPT_HASH:
 *       printf("Voter is a DRep identified by script hash.\n");
 *       break;
 *     case CARDANO_VOTER_TYPE_STAKE_POOL_KEY_HASH:
 *       printf("Voter is a Stake Pool Operator identified by key hash.\n");
 *       break;
 *   }
 * }
 *
 * // Clean up the voter object once done
 * cardano_voter_unref(&voter);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_voter_get_type(
  const cardano_voter_t* voter,
  cardano_voter_type_t*  type);

/**
 * \brief Retrieves the credential associated with a voter.
 *
 * This function returns the credential of a \ref cardano_voter_t object.
 * It returns a new reference to a \ref cardano_credential_t object representing the voter's credential.
 * It is the caller's responsibility to release it by calling \ref cardano_credential_unref when
 * it is no longer needed.
 *
 * \param[in] voter A constant pointer to the \ref cardano_voter_t object from which
 *                   the credential is to be retrieved.
 *
 * \return A pointer to a new \ref cardano_credential_t object containing the voter's credential.
 *         If the input voter is NULL, returns NULL. The caller is responsible for
 *         managing the lifecycle of this object, including releasing it with
 *         \ref cardano_credential_unref.
 *
 * Usage Example:
 * \code{.c}
 * cardano_voter_t* original_voter = cardano_voter_new(...);
 * cardano_credential_t* credential = cardano_voter_get_credential(original_voter);
 *
 * if (credential)
 * {
 *   // Use the credential
 *
 *   // Once done, ensure to clean up and release the credential
 *   cardano_credential_unref(&credential);
 * }
 * // Release the original voter after use
 * cardano_voter_unref(&original_voter);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_credential_t* cardano_voter_get_credential(cardano_voter_t* voter);

/**
 * \brief Sets the credential for a voter.
 *
 * This function associates a credential with a \ref cardano_voter_t object. The function increases the reference
 * count of the credential, meaning the original credential must still be managed by the caller.
 *
 * \param[in] voter A pointer to the \ref cardano_voter_t object to which the credential will be set.
 * \param[in] credential A pointer to the \ref cardano_credential_t object that will be associated with the voter.
 *
 * \return \ref CARDANO_SUCCESS if the credential was successfully set, or an appropriate error code
 *         indicating the failure reason, such as \ref CARDANO_ERROR_POINTER_IS_NULL if any of the input pointers are NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_voter_t* voter = cardano_voter_new(...);
 * cardano_credential_t* credential = cardano_credential_new(...);
 *
 * cardano_error_t result = cardano_voter_set_credential(voter, credential);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Credential has been successfully set to the voter
 * }
 *
 * // Clean up
 * cardano_credential_unref(&credential);  // Decrease the reference since it was created separately
 * cardano_voter_unref(&voter);            // Release the voter after use
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_voter_set_credential(cardano_voter_t* voter, cardano_credential_t* credential);

/**
 * \brief Checks if two voter objects are equal.
 *
 * This function compares two \ref cardano_voter_t objects for equality.
 * It checks if the contents of the two voter objects are identical.
 *
 * \param[in] lhs A constant pointer to the first \ref cardano_voter_t object to be compared.
 * \param[in] rhs A constant pointer to the second \ref cardano_voter_t object to be compared.
 *
 * \return \c true if the two voter objects are equal (have the same contents), \c false otherwise.
 *
 * Usage Example:
 * \code{.c}
 * cardano_voter_t* voter1 = ...; // Assume voter1 is initialized
 * cardano_voter_t* voter2 = ...; // Assume voter2 is initialized
 *
 * if (cardano_voter_equals(voter1, voter2))
 * {
 *   printf("voter1 is equal to voter2\n");
 * }
 * else
 * {
 *   printf("voter1 is not equal to voter2\n");
 * }
 *
 * // Clean up the voter objects once done
 * cardano_voter_unref(&voter1);
 * cardano_voter_unref(&voter2);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT bool cardano_voter_equals(const cardano_voter_t* lhs, const cardano_voter_t* rhs);

/**
 * \brief Decrements the reference count of a voter object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_voter_t object
 * by decreasing its reference count. When the reference count reaches zero, the voter is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] voter A pointer to the pointer of the voter object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_voter_t* voter = cardano_voter_new();
 *
 * // Perform operations with the voter...
 *
 * cardano_voter_unref(&voter);
 * // At this point, voter is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_voter_unref, the pointer to the \ref cardano_voter_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_voter_unref(cardano_voter_t** voter);

/**
 * \brief Increases the reference count of the cardano_voter_t object.
 *
 * This function is used to manually increment the reference count of a voter
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_voter_unref.
 *
 * \param voter A pointer to the voter object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming voter is a previously created voter object
 *
 * cardano_voter_ref(voter);
 *
 * // Now voter can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_voter_ref there is a corresponding
 * call to \ref cardano_voter_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_voter_ref(cardano_voter_t* voter);

/**
 * \brief Retrieves the current reference count of the cardano_voter_t object.
 *
 * This function returns the number of active references to a voter object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_voter_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param voter A pointer to the voter object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified voter object. If the object
 * is properly managed (i.e., every \ref cardano_voter_ref call is matched with a
 * \ref cardano_voter_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming voter is a previously created voter object
 *
 * size_t ref_count = cardano_voter_refcount(voter);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_voter_refcount(const cardano_voter_t* voter);

/**
 * \brief Sets the last error message for a given voter object.
 *
 * Records an error message in the voter's last_error buffer, overwriting any existing message.
 * This is useful for storing devoterive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] voter A pointer to the \ref cardano_voter_t instance whose last error message is
 *                       to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the voter's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_voter_set_last_error(cardano_voter_t* voter, const char* message);

/**
 * \brief Retrieves the last error message recorded for a specific voter.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_voter_set_last_error for the given
 * voter. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] voter A pointer to the \ref cardano_voter_t instance whose last error
 *                   message is to be retrieved. If the voter is NULL, the function
 *                   returns a generic error message indicating the null voter.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified voter. If the voter is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_voter_set_last_error for the same voter, or until
 *       the voter is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_voter_get_last_error(const cardano_voter_t* voter);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_VOTER_H