/**
 * \file mir_cert.h
 *
 * \author angel.castillo
 * \date   Jun 19, 2024
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_MIR_CERT_H
#define BIGLUP_LABS_INCLUDE_CARDANO_MIR_CERT_H

/* INCLUDES ******************************************************************/

#include <cardano/cbor/cbor_reader.h>
#include <cardano/cbor/cbor_writer.h>
#include <cardano/certs/mir_cert_type.h>
#include <cardano/error.h>
#include <cardano/export.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Certificate used to facilitate an instantaneous transfer of rewards within the system.
 *
 * Typically, rewards in Cardano are accumulated and distributed through a carefully designed
 * process aligned with the staking and delegation mechanics. However, certain situations may
 * require a more immediate or specialized handling of rewards, and that's where this type of
 * certificate comes into play.
 *
 * The MoveInstantaneousReward certificate allows for immediate redistribution of rewards
 * within pots, or to a specified se of stake addresses.
 */
typedef struct cardano_mir_cert_t cardano_mir_cert_t;

/**
 * \brief This certificate move instantaneous rewards funds between accounting pots.
 */
typedef struct cardano_mir_to_pot_cert_t cardano_mir_to_pot_cert_t;

/**
 * \brief Creates a move instantaneous rewards certificate that transfers funds to the given set of reward accounts.
 */
typedef struct cardano_mir_to_stake_creds_cert_t cardano_mir_to_stake_creds_cert_t;

/**
 * \brief Certificates are a means to encode various essential operations related to stake
 * delegation and stake pool management. Certificates are embedded in transactions and
 * included in blocks. They're a vital aspect of Cardano's proof-of-stake mechanism,
 * ensuring that stakeholders can participate in the protocol and its governance.
 */
typedef struct cardano_certificate_t cardano_certificate_t;

/**
 * \brief Creates a Move Instantaneous Rewards (MIR) certificate for transferring funds to another accounting pot.
 *
 * This function initializes a new instance of \ref cardano_mir_cert_t that represents a certificate for moving
 * instantaneous rewards between different accounting pots within the Cardano system.
 *
 * \param[in] to_other_pot_cert A pointer to the \ref cardano_mir_to_pot_cert_t object to cast as a \ref cardano_mir_cert_t.
 * \param[out] mir_cert A pointer to a pointer that will be set to the address of the newly created \ref cardano_mir_cert_t object.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the certificate
 *         was successfully created, or an appropriate error code indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_mir_to_pot_cert_t* to_other_pot_cert = NULL;
 * cardano_mir_cert_t* mir_cert = NULL;
 *
 * cardano_error_t result = cardano_mir_cert_new_to_other_pot(&to_other_pot_cert, &mir_cert);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the MIR certificate
 *
 *   // Clean up
 *   cardano_mir_to_pot_cert_unref(&to_other_pot_cert);
 *   cardano_mir_cert_unref(&mir_cert);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_mir_cert_new_to_other_pot(
  cardano_mir_to_pot_cert_t* to_other_pot_cert,
  cardano_mir_cert_t**       mir_cert);

/**
 * \brief Creates a Move Instantaneous Rewards (MIR) certificate for transferring funds to a set of reward accounts.
 *
 * This function initializes a new instance of \ref cardano_mir_cert_t that represents a certificate for moving
 * instantaneous rewards to specified stake credentials within the Cardano system.
 *
 * \param[in] to_stake_creds_cert A pointer to a \ref cardano_mir_to_stake_creds_cert_t object representing the target stake credentials.
 * \param[out] mir_cert A pointer to a pointer that will be set to the address of the newly created \ref cardano_mir_cert_t object.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the certificate
 *         was successfully created, or an appropriate error code indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_mir_to_stake_creds_cert_t* to_stake_creds_cert = ...; // Assume this is initialized
 * cardano_mir_cert_t* mir_cert = NULL;
 *
 * cardano_error_t result = cardano_mir_cert_new_to_stake_creds(to_stake_creds_cert, &mir_cert);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the MIR certificate
 *
 *   // Clean up
 *   cardano_mir_to_stake_creds_cert_unref(&to_stake_creds_cert);
 *   cardano_mir_cert_unref(&mir_cert);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_mir_cert_new_to_stake_creds(
  cardano_mir_to_stake_creds_cert_t* to_stake_creds_cert,
  cardano_mir_cert_t**               mir_cert);

/**
 * \brief Creates a \ref cardano_mir_cert_t from a CBOR reader.
 *
 * This function parses CBOR data using a provided \ref cardano_cbor_reader_t and constructs a \ref cardano_mir_cert_t object.
 * It assumes that the CBOR reader is set up correctly and that the CBOR data corresponds to the structure expected for a mir_cert.
 *
 * \param[in] reader A pointer to an initialized \ref cardano_cbor_reader_t that is ready to read the CBOR-encoded data.
 * \param[out] mir_cert A pointer to a pointer of \ref cardano_mir_cert_t that will be set to the address
 *                        of the newly created mir_cert object upon successful decoding.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the object was successfully created, or an appropriate error code if an error occurred.
 *
 * \note If the function fails, the last error can be retrieved by calling \ref cardano_cbor_reader_get_last_error with the reader.
 *       The caller is responsible for freeing the created \ref cardano_mir_cert_t object by calling
 *       \ref cardano_mir_cert_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_new(cbor_data, data_size);
 * cardano_mir_cert_t* mir_cert = NULL;
 *
 * cardano_error_t result = cardano_mir_cert_from_cbor(reader, &mir_cert);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the mir_cert
 *
 *   // Once done, ensure to clean up and release the mir_cert
 *   cardano_mir_cert_unref(&mir_cert);
 * }
 * else
 * {
 *   const char* error = cardano_cbor_reader_get_last_error(reader);
 *   printf("Failed to decode mir_cert: %s\n", error);
 * }
 *
 * cardano_cbor_reader_unref(&reader); // Cleanup the CBOR reader
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_mir_cert_from_cbor(cardano_cbor_reader_t* reader, cardano_mir_cert_t** mir_cert);

/**
 * \brief Serializes the certificate into CBOR format using a CBOR writer.
 *
 * This function serializes the given \ref cardano_mir_cert_t object using a \ref cardano_cbor_writer_t.
 *
 * \param[in] mir_cert A constant pointer to the \ref cardano_mir_cert_t object that is to be serialized.
 * \param[out] writer A pointer to a \ref cardano_cbor_writer_t where the CBOR serialized data will be written.
 *                    The writer must already be initialized and ready to accept the data.
 *
 * \return Returns \ref CARDANO_SUCCESS if the serialization is successful. If the \p mir_cert or \p writer
 *         is NULL, returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_mir_cert_t* mir_cert = ...;
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * if (writer)
 * {
 *   cardano_error_t result = cardano_mir_cert_to_cbor(mir_cert, writer);
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
 * cardano_mir_cert_unref(&mir_cert);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_mir_cert_to_cbor(
  const cardano_mir_cert_t* mir_cert,
  cardano_cbor_writer_t*    writer);

/**
 * \brief Retrieves the type of a Move Instantaneous Rewards (MIR) certificate.
 *
 * This function extracts the type of the specified \ref cardano_mir_cert_t object, which indicates
 * whether the certificate moves funds between accounting pots or transfers funds to specified
 * reward accounts.
 *
 * \param[in] mir_cert A constant pointer to the \ref cardano_mir_cert_t object whose type is to be retrieved.
 * \param[out] type A pointer to a \ref cardano_mir_cert_type_t variable where the type of the MIR certificate will be stored.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the type
 *         was successfully retrieved, or an appropriate error code indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_mir_cert_t* mir_cert = ...; // Assume this is initialized
 * cardano_mir_cert_type_t type;
 *
 * cardano_error_t result = cardano_mir_cert_get_type(mir_cert, &type);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the MIR certificate type
 *   if (type == CARDANO_MIR_CERT_TYPE_TO_POT)
 *   {
 *     // Handle pot type
 *   }
 *   else if (type == CARDANO_MIR_CERT_TYPE_TO_STAKE_CREDS)
 *   {
 *     // Handle stake credentials type
 *   }
 * }
 *
 * // Clean up
 * cardano_mir_cert_unref(&mir_cert);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_mir_cert_get_type(const cardano_mir_cert_t* mir_cert, cardano_mir_cert_type_t* type);

/**
 * \brief Retrieves the Move Instantaneous Rewards (MIR) certificate as a 'to other pot' certificate.
 *
 * This function extracts the \ref cardano_mir_to_pot_cert_t object from the specified
 * \ref cardano_mir_cert_t object if the MIR certificate type is \ref CARDANO_MIR_CERT_TYPE_TO_POT.
 *
 * \param[in] mir_cert A pointer to the \ref cardano_mir_cert_t object to be converted.
 * \param[out] to_other_pot_cert A pointer to a pointer that will be set to the address of the
 *             \ref cardano_mir_to_pot_cert_t object.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the conversion was successful, or an appropriate error code indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_mir_cert_t* mir_cert = ...; // Assume this is initialized
 * cardano_mir_to_pot_cert_t* to_other_pot_cert = NULL;
 *
 * cardano_error_t result = cardano_mir_cert_as_to_other_pot(mir_cert, &to_other_pot_cert);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the to_other_pot_cert
 * }
 *
 * // Clean up
 * cardano_mir_cert_unref(&mir_cert);
 * cardano_mir_to_pot_cert_unref(&to_other_pot_cert);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_mir_cert_as_to_other_pot(
  cardano_mir_cert_t*         mir_cert,
  cardano_mir_to_pot_cert_t** to_other_pot_cert);

/**
 * \brief Retrieves the Move Instantaneous Rewards (MIR) certificate as a 'to stake credentials' certificate.
 *
 * This function extracts the \ref cardano_mir_to_stake_creds_cert_t object from the specified
 * \ref cardano_mir_cert_t object if the MIR certificate type is \ref CARDANO_MIR_CERT_TYPE_TO_STAKE_CREDS.
 *
 * \param[in] mir_cert A pointer to the \ref cardano_mir_cert_t object to be converted.
 * \param[out] to_stake_creds_cert A pointer to a pointer that will be set to the address of the
 *             \ref cardano_mir_to_stake_creds_cert_t object.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the conversion was successful, or an appropriate error code indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_mir_cert_t* mir_cert = ...; // Assume this is initialized
 * cardano_mir_to_stake_creds_cert_t* to_stake_creds_cert = NULL;
 *
 * cardano_error_t result = cardano_mir_cert_as_to_stake_creds(mir_cert, &to_stake_creds_cert);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the to_stake_creds_cert
 * }
 *
 * // Clean up
 * cardano_mir_cert_unref(&mir_cert);
 * cardano_mir_to_stake_creds_cert_unref(&to_stake_creds_cert);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_mir_cert_as_to_stake_creds(
  cardano_mir_cert_t*                 mir_cert,
  cardano_mir_to_stake_creds_cert_t** to_stake_creds_cert);

/**
 * \brief Decrements the reference count of a cardano_mir_cert_t object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_mir_cert_t object
 * by decreasing its reference count. When the reference count reaches zero, the mir_cert is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] mir_cert A pointer to the pointer of the mir_cert object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_mir_cert_t* mir_cert = cardano_mir_cert_new(major, minor);
 *
 * // Perform operations with the mir_cert...
 *
 * cardano_mir_cert_unref(&mir_cert);
 * // At this point, mir_cert is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_mir_cert_unref, the pointer to the \ref cardano_mir_cert_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_mir_cert_unref(cardano_mir_cert_t** mir_cert);

/**
 * \brief Increases the reference count of the cardano_mir_cert_t object.
 *
 * This function is used to manually increment the reference count of an cardano_mir_cert_t
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_mir_cert_unref.
 *
 * \param mir_cert A pointer to the cardano_mir_cert_t object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming mir_cert is a previously created mir_cert object
 *
 * cardano_mir_cert_ref(mir_cert);
 *
 * // Now mir_cert can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_mir_cert_ref there is a corresponding
 * call to \ref cardano_mir_cert_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_mir_cert_ref(cardano_mir_cert_t* mir_cert);

/**
 * \brief Retrieves the current reference count of the cardano_mir_cert_t object.
 *
 * This function returns the number of active references to an cardano_mir_cert_t object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_mir_cert_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param mir_cert A pointer to the cardano_mir_cert_t object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified cardano_mir_cert_t object. If the object
 * is properly managed (i.e., every \ref cardano_mir_cert_ref call is matched with a
 * \ref cardano_mir_cert_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming mir_cert is a previously created mir_cert object
 *
 * size_t ref_count = cardano_mir_cert_refcount(mir_cert);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_mir_cert_refcount(const cardano_mir_cert_t* mir_cert);

/**
 * \brief Sets the last error message for a given cardano_mir_cert_t object.
 *
 * Records an error message in the mir_cert's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] mir_cert A pointer to the \ref cardano_mir_cert_t instance whose last error message is
 *                       to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the mir_cert's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_mir_cert_set_last_error(
  cardano_mir_cert_t* mir_cert,
  const char*         message);

/**
 * \brief Retrieves the last error message recorded for a specific mir_cert.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_mir_cert_set_last_error for the given
 * mir_cert. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] mir_cert A pointer to the \ref cardano_mir_cert_t instance whose last error
 *                   message is to be retrieved. If the mir_cert is NULL, the function
 *                   returns a generic error message indicating the null mir_cert.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified mir_cert. If the mir_cert is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_mir_cert_set_last_error for the same mir_cert, or until
 *       the mir_cert is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_mir_cert_get_last_error(
  const cardano_mir_cert_t* mir_cert);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_MIR_CERT_H