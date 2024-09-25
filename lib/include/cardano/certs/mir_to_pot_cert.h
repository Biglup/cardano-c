/**
 * \file mir_to_pot_cert.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_MIR_TO_POD_CERT_H
#define BIGLUP_LABS_INCLUDE_CARDANO_MIR_TO_POD_CERT_H

/* INCLUDES ******************************************************************/

#include <cardano/cbor/cbor_reader.h>
#include <cardano/cbor/cbor_writer.h>
#include <cardano/certs/mir_cert_pot_type.h>
#include <cardano/error.h>
#include <cardano/export.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief This certificate move instantaneous rewards funds between accounting pots.
 */
typedef struct cardano_mir_to_pot_cert_t cardano_mir_to_pot_cert_t;

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
 * \brief Initializes a new instance of the Move Instantaneous Reward (MIR) certificate for transferring funds between pots.
 *
 * This function initializes a new \ref cardano_mir_to_pot_cert_t object, which represents a MIR certificate
 * designed to transfer a specified amount of ADA from one accounting pot to another within the Cardano ecosystem.
 *
 * \param[in] pot_type Determines the accounting pot from which the funds will be drawn. This is specified by the
 *                     \ref cardano_mir_to_pot_cert_t enumeration, which includes options for the reserve pot or the treasury pot.
 * \param[in] amount The amount of ADA to be transferred by this MIR certificate.
 * \param[out] mir_to_pot_cert On successful creation, this will point to a newly created \ref cardano_mir_to_pot_cert_t object.
 *                             The caller is responsible for managing the lifecycle of this object. Specifically,
 *                             once the MIR certificate is no longer needed, the caller must release it by calling
 *                             the appropriate deallocation function.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the MIR certificate was successfully created, or an appropriate error code indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_mir_to_pot_cert_t* mir_to_pot_cert = NULL;
 * cardano_error_t result = cardano_mir_to_pot_cert_new(CARDANO_MIR_CERT_POT_TYPE_RESERVE, 1000, &mir_to_pot_cert);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the MIR certificate
 *
 *   // Once done, ensure to clean up and release the MIR certificate
 *   cardano_mir_to_pot_cert_unref(&mir_to_pot_cert);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_mir_to_pot_cert_new(
  cardano_mir_cert_pot_type_t pot_type,
  uint64_t                    amount,
  cardano_mir_to_pot_cert_t** mir_to_pot_cert);

/**
 * \brief Creates a \ref cardano_mir_to_pot_cert_t from a CBOR reader.
 *
 * This function parses CBOR data using a provided \ref cardano_cbor_reader_t and constructs a \ref cardano_mir_to_pot_cert_t object.
 * It assumes that the CBOR reader is set up correctly and that the CBOR data corresponds to the structure expected for a mir_to_pot_cert.
 *
 * \param[in] reader A pointer to an initialized \ref cardano_cbor_reader_t that is ready to read the CBOR-encoded data.
 * \param[out] mir_to_pot_cert A pointer to a pointer of \ref cardano_mir_to_pot_cert_t that will be set to the address
 *                        of the newly created mir_to_pot_cert object upon successful decoding.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the object was successfully created, or an appropriate error code if an error occurred.
 *
 * \note If the function fails, the last error can be retrieved by calling \ref cardano_cbor_reader_get_last_error with the reader.
 *       The caller is responsible for freeing the created \ref cardano_mir_to_pot_cert_t object by calling
 *       \ref cardano_mir_to_pot_cert_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_new(cbor_data, data_size);
 * cardano_mir_to_pot_cert_t* mir_to_pot_cert = NULL;
 *
 * cardano_error_t result = cardano_mir_to_pot_cert_from_cbor(reader, &mir_to_pot_cert);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the mir_to_pot_cert
 *
 *   // Once done, ensure to clean up and release the mir_to_pot_cert
 *   cardano_mir_to_pot_cert_unref(&mir_to_pot_cert);
 * }
 * else
 * {
 *   const char* error = cardano_cbor_reader_get_last_error(reader);
 *   printf("Failed to decode mir_to_pot_cert: %s\n", error);
 * }
 *
 * cardano_cbor_reader_unref(&reader); // Cleanup the CBOR reader
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_mir_to_pot_cert_from_cbor(cardano_cbor_reader_t* reader, cardano_mir_to_pot_cert_t** mir_to_pot_cert);

/**
 * \brief Serializes the certificate into CBOR format using a CBOR writer.
 *
 * This function serializes the given \ref cardano_mir_to_pot_cert_t object using a \ref cardano_cbor_writer_t.
 *
 * \param[in] mir_to_pot_cert A constant pointer to the \ref cardano_mir_to_pot_cert_t object that is to be serialized.
 * \param[out] writer A pointer to a \ref cardano_cbor_writer_t where the CBOR serialized data will be written.
 *                    The writer must already be initialized and ready to accept the data.
 *
 * \return Returns \ref CARDANO_SUCCESS if the serialization is successful. If the \p mir_to_pot_cert or \p writer
 *         is NULL, returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_mir_to_pot_cert_t* mir_to_pot_cert = ...;
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * if (writer)
 * {
 *   cardano_error_t result = cardano_mir_to_pot_cert_to_cbor(mir_to_pot_cert, writer);
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
 * cardano_mir_to_pot_cert_unref(&mir_to_pot_cert);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_mir_to_pot_cert_to_cbor(
  const cardano_mir_to_pot_cert_t* mir_to_pot_cert,
  cardano_cbor_writer_t*           writer);

/**
 * \brief Retrieves the pot type from a Move Instantaneous Reward (MIR) certificate.
 *
 * This function extracts the pot type from the given \ref cardano_mir_to_pot_cert_t object, indicating whether the funds
 * are drawn from the reserve or the treasury.
 *
 * \param[in] mir_cert A constant pointer to the \ref cardano_mir_to_pot_cert_t object.
 * \param[out] type A pointer to a \ref cardano_mir_cert_pot_type_t variable where the pot type will be stored.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the pot type was successfully retrieved, or an appropriate error code indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_mir_to_pot_cert_t* mir_cert = ...; // Assume this is initialized
 * cardano_mir_cert_pot_type_t pot_type;
 * cardano_error_t result = cardano_mir_to_pot_cert_get_pot(mir_cert, &pot_type);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the pot type
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_mir_to_pot_cert_get_pot(const cardano_mir_to_pot_cert_t* mir_cert, cardano_mir_cert_pot_type_t* type);

/**
 * \brief Sets the pot type for a Move Instantaneous Reward (MIR) certificate.
 *
 * This function assigns the specified pot type to the given \ref cardano_mir_to_pot_cert_t object, indicating whether the funds
 * are drawn from the reserve or the treasury.
 *
 * \param[in,out] mir_cert A pointer to the \ref cardano_mir_to_pot_cert_t object.
 * \param[in] type The pot type to be set, represented by \ref cardano_mir_cert_pot_type_t.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the pot type was successfully set, or an appropriate error code indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_mir_to_pot_cert_t* mir_cert = ...; // Assume this is initialized
 * cardano_mir_cert_pot_type_t pot_type = CARDANO_MIR_CERT_POT_TYPE_RESERVE;
 * cardano_error_t result = cardano_mir_to_pot_cert_set_pot(mir_cert, pot_type);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Pot type was successfully set
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_mir_to_pot_cert_set_pot(cardano_mir_to_pot_cert_t* mir_cert, cardano_mir_cert_pot_type_t type);

/**
 * \brief Retrieves the amount specified in a Move Instantaneous Reward (MIR) certificate.
 *
 * This function gets the amount to be transferred as specified in the given \ref cardano_mir_to_pot_cert_t object.
 *
 * \param[in] mir_cert A constant pointer to the \ref cardano_mir_to_pot_cert_t object.
 * \param[out] amount A pointer to a uint64_t where the amount will be stored.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the amount was successfully retrieved, or an appropriate error code indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * const cardano_mir_to_pot_cert_t* mir_cert = ...; // Assume this is initialized
 * uint64_t amount = 0;
 * cardano_error_t result = cardano_mir_to_pot_cert_get_amount(mir_cert, &amount);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Amount was successfully retrieved
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_mir_to_pot_cert_get_amount(const cardano_mir_to_pot_cert_t* mir_cert, uint64_t* amount);

/**
 * \brief Sets the amount in a Move Instantaneous Reward (MIR) certificate.
 *
 * This function sets the amount to be transferred in the given \ref cardano_mir_to_pot_cert_t object.
 *
 * \param[in,out] mir_cert A pointer to the \ref cardano_mir_to_pot_cert_t object.
 * \param[in] amount The amount to be set in the MIR certificate.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the amount was successfully set, or an appropriate error code indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * cardano_mir_to_pot_cert_t* mir_cert = ...; // Assume this is initialized
 * uint64_t amount = 1000;
 * cardano_error_t result = cardano_mir_to_pot_cert_set_amount(mir_cert, amount);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Amount was successfully set
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_mir_to_pot_cert_set_amount(cardano_mir_to_pot_cert_t* mir_cert, uint64_t amount);

/**
 * \brief Decrements the reference count of a cardano_mir_to_pot_cert_t object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_mir_to_pot_cert_t object
 * by decreasing its reference count. When the reference count reaches zero, the mir_to_pot_cert is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] mir_to_pot_cert A pointer to the pointer of the mir_to_pot_cert object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_mir_to_pot_cert_t* mir_to_pot_cert = cardano_mir_to_pot_cert_new(major, minor);
 *
 * // Perform operations with the mir_to_pot_cert...
 *
 * cardano_mir_to_pot_cert_unref(&mir_to_pot_cert);
 * // At this point, mir_to_pot_cert is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_mir_to_pot_cert_unref, the pointer to the \ref cardano_mir_to_pot_cert_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_mir_to_pot_cert_unref(cardano_mir_to_pot_cert_t** mir_to_pot_cert);

/**
 * \brief Increases the reference count of the cardano_mir_to_pot_cert_t object.
 *
 * This function is used to manually increment the reference count of an cardano_mir_to_pot_cert_t
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_mir_to_pot_cert_unref.
 *
 * \param mir_to_pot_cert A pointer to the cardano_mir_to_pot_cert_t object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming mir_to_pot_cert is a previously created mir_to_pot_cert object
 *
 * cardano_mir_to_pot_cert_ref(mir_to_pot_cert);
 *
 * // Now mir_to_pot_cert can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_mir_to_pot_cert_ref there is a corresponding
 * call to \ref cardano_mir_to_pot_cert_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_mir_to_pot_cert_ref(cardano_mir_to_pot_cert_t* mir_to_pot_cert);

/**
 * \brief Retrieves the current reference count of the cardano_mir_to_pot_cert_t object.
 *
 * This function returns the number of active references to an cardano_mir_to_pot_cert_t object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_mir_to_pot_cert_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param mir_to_pot_cert A pointer to the cardano_mir_to_pot_cert_t object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified cardano_mir_to_pot_cert_t object. If the object
 * is properly managed (i.e., every \ref cardano_mir_to_pot_cert_ref call is matched with a
 * \ref cardano_mir_to_pot_cert_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming mir_to_pot_cert is a previously created mir_to_pot_cert object
 *
 * size_t ref_count = cardano_mir_to_pot_cert_refcount(mir_to_pot_cert);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_mir_to_pot_cert_refcount(const cardano_mir_to_pot_cert_t* mir_to_pot_cert);

/**
 * \brief Sets the last error message for a given cardano_mir_to_pot_cert_t object.
 *
 * Records an error message in the mir_to_pot_cert's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] mir_to_pot_cert A pointer to the \ref cardano_mir_to_pot_cert_t instance whose last error message is
 *                       to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the mir_to_pot_cert's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_mir_to_pot_cert_set_last_error(
  cardano_mir_to_pot_cert_t* mir_to_pot_cert,
  const char*                message);

/**
 * \brief Retrieves the last error message recorded for a specific mir_to_pot_cert.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_mir_to_pot_cert_set_last_error for the given
 * mir_to_pot_cert. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] mir_to_pot_cert A pointer to the \ref cardano_mir_to_pot_cert_t instance whose last error
 *                   message is to be retrieved. If the mir_to_pot_cert is NULL, the function
 *                   returns a generic error message indicating the null mir_to_pot_cert.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified mir_to_pot_cert. If the mir_to_pot_cert is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_mir_to_pot_cert_set_last_error for the same mir_to_pot_cert, or until
 *       the mir_to_pot_cert is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_mir_to_pot_cert_get_last_error(
  const cardano_mir_to_pot_cert_t* mir_to_pot_cert);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_MIR_TO_POD_CERT_H