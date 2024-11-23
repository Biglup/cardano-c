/**
 * \file parameter_change_action.h
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

#ifndef PARAMETER_CHANGE_ACTION_H
#define PARAMETER_CHANGE_ACTION_H

/* INCLUDES ******************************************************************/

#include <cardano/cbor/cbor_reader.h>
#include <cardano/cbor/cbor_writer.h>
#include <cardano/common/governance_action_id.h>
#include <cardano/crypto/blake2b_hash.h>
#include <cardano/error.h>
#include <cardano/export.h>
#include <cardano/proposal_procedures/constitution.h>
#include <cardano/protocol_params/protocol_param_update.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Updates one or more updatable protocol parameters, excluding changes to major protocol versions (i.e., "hard forks").
 */
typedef struct cardano_parameter_change_action_t cardano_parameter_change_action_t;

/**
 * \brief Creates and initializes a new instance of the Parameter Change Action.
 *
 * This function allocates and initializes a new instance of a \ref cardano_parameter_change_action_t object,
 * which represents an action to update one or more updatable protocol parameters within the Cardano network.
 * These updates exclude major protocol version changes, which are managed through different actions.
 *
 * **Governance Action ID:**
 * The action requires a governance action ID to reference the most recent enacted action of the
 * same type. This is necessary to prevent unintended conflicts between governance actions of the same type.
 * You can retrieve this information from the governance state query:
 *
 * \code{.sh}
 * cardano-cli conway query gov-state | jq .nextRatifyState.nextEnactState.prevGovActionIds
 * \endcode
 *
 * Example output:
 * \code{.json}
 * {
 *   "Committee": {
 *     "govActionIx": 0,
 *     "txId": "6bff8515060c08e9cae4d4e203a4d8b2e876848aae8c4e896acda7202d3ac679"
 *   },
 *   "Constitution": null,
 *   "HardFork": null,
 *   "PParamUpdate": {
 *     "govActionIx": 0,
 *     "txId": "7e199d036f1e8d725ea8aba30c5f8d0d2ab9dbd45c7f54e7d85c92c022673f0f"
 *   }
 * }
 * \endcode
 *
 * **Guardrails Script Hash:**
 * The `policy_hash` parameter represents the hash of the guardrails script (also known as the governance action policy script).
 * The guardrails script is a Plutus script that acts as a safeguard by imposing additional constraints on certain types
 * of governance actions, such as protocol parameter updates and treasury withdrawals. When proposing a protocol parameter update, you must
 * provide the guardrails script hash to reference it. This ensures that the proposal is validated against the guardrails script during the transaction processing.
 *
 * You can obtain the guardrails script hash using the `cardano-cli`:
 * \code{.sh}
 * cardano-cli hash script --script-file guardrails-script.plutus
 * \endcode
 *
 * Example output:
 * \code{.sh}
 * fa24fb305126805cf2164c161d852a0e7330cf988f1fe558cf7d4a64
 * \endcode
 *
 * \param[in] protocol_param_update A pointer to a \ref cardano_protocol_param_update_t object representing the protocol parameter updates.
 *                                  This object should include the parameters you wish to update.
 * \param[in] governance_action_id An optional pointer to a \ref cardano_governance_action_id_t object representing the last enacted governance
 *                                 action of the same type (Protocol Parameter Update). This parameter can be `NULL` if no governance action of this type has been enacted.
 * \param[in] policy_hash An optional pointer to a \ref cardano_blake2b_hash_t object representing the hash of the guardrails script.
 * \param[out] parameter_change_action On successful initialization, this will point to a newly created
 *             \ref cardano_parameter_change_action_t object. This object represents a "strong reference"
 *             to the parameter change action, meaning that it is fully initialized and ready for use.
 *             The caller is responsible for managing the lifecycle of this object;
 *             specifically, once the parameter change action is no longer needed, the caller must release it
 *             by calling \ref cardano_parameter_change_action_unref.
 *
 * **Return Value:**
 * \return \c cardano_error_t indicating the outcome of the operation. Returns \c CARDANO_SUCCESS if the parameter change action was
 *         successfully created, or an appropriate error code indicating the failure reason.
 *
 * **Usage Example:**
 * \code{.c}
 * // Initialize the protocol parameter updates
 * cardano_protocol_param_update_t* protocol_param_update = cardano_protocol_param_update_new();
 * // Set the parameters you wish to update, for example:
 * cardano_protocol_param_update_set_key_deposit(protocol_param_update, 1000000);
 *
 * // Retrieve the last enacted governance action ID (if any)
 * cardano_governance_action_id_t* governance_action_id = cardano_governance_action_id_new(0, "7e199d036f1e8d725ea8aba30c5f8d0d2ab9dbd45c7f54e7d85c92c022673f0f");
 *
 * // Obtain the guardrails script hash (if required)
 * cardano_blake2b_hash_t* policy_hash = cardano_blake2b_hash_from_hex("fa24fb305126805cf2164c161d852a0e7330cf988f1fe558cf7d4a64");
 *
 * cardano_parameter_change_action_t* parameter_change_action = NULL;
 * cardano_error_t result = cardano_parameter_change_action_new(
 *   protocol_param_update,
 *   governance_action_id,
 *   policy_hash,
 *   &parameter_change_action);
 *
 * // Clean up
 * cardano_protocol_param_update_unref(&protocol_param_update);
 * cardano_governance_action_id_unref(&governance_action_id);
 * cardano_blake2b_hash_unref(&policy_hash);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_parameter_change_action_new(
  cardano_protocol_param_update_t*    protocol_param_update,
  cardano_governance_action_id_t*     governance_action_id,
  cardano_blake2b_hash_t*             policy_hash,
  cardano_parameter_change_action_t** parameter_change_action);

/**
 * \brief Creates a \ref cardano_parameter_change_action_t from a CBOR reader.
 *
 * This function parses CBOR data using a provided \ref cardano_cbor_reader_t and constructs a \ref cardano_parameter_change_action_t object.
 * It assumes that the CBOR reader is set up correctly and that the CBOR data corresponds to the structure expected for a parameter_change_action.
 *
 * \param[in] reader A pointer to an initialized \ref cardano_cbor_reader_t that is ready to read the CBOR-encoded data.
 * \param[out] parameter_change_action A pointer to a pointer of \ref cardano_parameter_change_action_t that will be set to the address
 *                        of the newly created parameter_change_action object upon successful decoding.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the object was successfully created, or an appropriate error code if an error occurred.
 *
 * \note If the function fails, the last error can be retrieved by calling \ref cardano_cbor_reader_get_last_error with the reader.
 *       The caller is responsible for freeing the created \ref cardano_parameter_change_action_t object by calling
 *       \ref cardano_parameter_change_action_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_new(cbor_data, data_size);
 * cardano_parameter_change_action_t* parameter_change_action = NULL;
 *
 * cardano_error_t result = cardano_parameter_change_action_from_cbor(reader, &parameter_change_action);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the parameter_change_action
 *
 *   // Once done, ensure to clean up and release the parameter_change_action
 *   cardano_parameter_change_action_unref(&parameter_change_action);
 * }
 * else
 * {
 *   const char* error = cardano_cbor_reader_get_last_error(reader);
 *   printf("Failed to decode parameter_change_action: %s\n", error);
 * }
 *
 * cardano_cbor_reader_unref(&reader); // Cleanup the CBOR reader
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_parameter_change_action_from_cbor(cardano_cbor_reader_t* reader, cardano_parameter_change_action_t** parameter_change_action);

/**
 * \brief Serializes the certificate into CBOR format using a CBOR writer.
 *
 * This function serializes the given \ref cardano_parameter_change_action_t object using a \ref cardano_cbor_writer_t.
 *
 * \param[in] parameter_change_action A constant pointer to the \ref cardano_parameter_change_action_t object that is to be serialized.
 * \param[out] writer A pointer to a \ref cardano_cbor_writer_t where the CBOR serialized data will be written.
 *                    The writer must already be initialized and ready to accept the data.
 *
 * \return Returns \ref CARDANO_SUCCESS if the serialization is successful. If the \p parameter_change_action or \p writer
 *         is NULL, returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_parameter_change_action_t* parameter_change_action = ...;
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * if (writer)
 * {
 *   cardano_error_t result = cardano_parameter_change_action_to_cbor(parameter_change_action, writer);
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
 * cardano_parameter_change_action_unref(&parameter_change_action);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_parameter_change_action_to_cbor(
  const cardano_parameter_change_action_t* parameter_change_action,
  cardano_cbor_writer_t*                   writer);

/**
 * \brief Sets the protocol parameter update in the parameter change action.
 *
 * This function updates the protocol parameter update section of a \ref cardano_parameter_change_action_t object.
 * The protocol parameter update is a \ref cardano_protocol_param_update_t object representing the set of changes to the protocol parameters.
 *
 * \param[in,out] parameter_change_action A pointer to an initialized \ref cardano_parameter_change_action_t object to which the protocol parameter update will be set.
 * \param[in] protocol_param_update A pointer to an initialized \ref cardano_protocol_param_update_t object representing the new protocol parameter updates.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the protocol parameter update was
 *         successfully set, or an appropriate error code indicating the failure reason, such as \ref CARDANO_ERROR_POINTER_IS_NULL if the
 *         parameter_change_action pointer is NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_parameter_change_action_t* parameter_change_action = ...; // Assume parameter_change_action is already initialized
 * cardano_protocol_param_update_t* protocol_param_update = cardano_protocol_param_update_new(...); // Assume protocol_param_update is already initialized
 *
 * cardano_error_t result = cardano_parameter_change_action_set_protocol_param_update(parameter_change_action, protocol_param_update);
 * if (result == CARDANO_SUCCESS)
 * {
 *   // The protocol parameter updates are now set for the parameter_change_action
 * }
 * else
 * {
 *   printf("Failed to set the protocol parameter updates.\n");
 * }
 *
 * // Cleanup the parameter_change_action and protocol_param_update after use
 * cardano_parameter_change_action_unref(&parameter_change_action);
 * cardano_protocol_param_update_unref(&protocol_param_update);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_parameter_change_action_set_protocol_param_update(cardano_parameter_change_action_t* parameter_change_action, cardano_protocol_param_update_t* protocol_param_update);

/**
 * \brief Retrieves the protocol parameter updates from a parameter_change_action.
 *
 * This function retrieves the protocol parameter updates from a given \ref cardano_parameter_change_action_t object. The protocol parameter updates
 * are represented as a \ref cardano_protocol_param_update_t object.
 *
 * \param[in] parameter_change_action A pointer to an initialized \ref cardano_parameter_change_action_t object from which the protocol parameter updates are retrieved.
 *
 * \return A pointer to the retrieved \ref cardano_protocol_param_update_t object representing the protocol parameter updates.
 *         This will be a new reference, and the caller is responsible for releasing it with \ref cardano_protocol_param_update_unref
 *         when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_parameter_change_action_t* parameter_change_action = ...; // Assume initialized
 * cardano_protocol_param_update_t* protocol_param_update = cardano_parameter_change_action_get_protocol_param_update(parameter_change_action);
 *
 * if (protocol_param_update != NULL)
 * {
 *   // Use the protocol parameter updates
 *
 *   // Once done, ensure to clean up and release the protocol parameter updates
 *   cardano_protocol_param_update_unref(&protocol_param_update);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_protocol_param_update_t*
cardano_parameter_change_action_get_protocol_param_update(cardano_parameter_change_action_t* parameter_change_action);

/**
 * \brief Sets the guardrails script hash in the parameter change action.
 *
 * This function updates the guardrails script hash (also known as the policy hash) of a
 * \ref cardano_parameter_change_action_t object. The guardrails script is an optional Plutus script that
 * imposes additional constraints on certain types of governance actions, such as protocol parameter updates
 * and treasury withdrawals. By setting the guardrails script hash, you reference this script in your
 * parameter change action, ensuring that the proposal adheres to the constraints defined by the script.
 *
 * \param[in,out] parameter_change_action A pointer to an initialized \ref cardano_parameter_change_action_t object
 *                                        to which the guardrails script hash will be set.
 * \param[in] policy_hash A pointer to an initialized \ref cardano_blake2b_hash_t object representing the new
 *                        guardrails script hash. This parameter can be `NULL` if you wish to unset any previously
 *                        set guardrails script hash.
 *
 * **Return Value:**
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the guardrails
 *         script hash was successfully set, or an appropriate error code indicating the failure reason, such as
 *         \ref CARDANO_ERROR_POINTER_IS_NULL if the `parameter_change_action` pointer is `NULL`.
 *
 * **Usage Example:**
 * \code{.c}
 * // Assume parameter_change_action is already initialized
 * cardano_parameter_change_action_t* parameter_change_action = ...;
 *
 * // Obtain the guardrails script hash
 * cardano_blake2b_hash_t* policy_hash = cardano_blake2b_hash_from_hex("fa24fb305126805cf2164c161d852a0e7330cf988f1fe558cf7d4a64");
 *
 * // Set the guardrails script hash in the parameter change action
 * cardano_error_t result = cardano_parameter_change_action_set_policy_hash(parameter_change_action, policy_hash);
 * if (result == CARDANO_SUCCESS)
 * {
 *   // The guardrails script hash is now set for the parameter_change_action
 * }
 * else
 * {
 *   printf("Failed to set the guardrails script hash: %s\n", cardano_error_to_string(result));
 * }
 *
 * // Clean up after use
 * cardano_parameter_change_action_unref(&parameter_change_action);
 * cardano_blake2b_hash_unref(&policy_hash);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_parameter_change_action_set_policy_hash(cardano_parameter_change_action_t* parameter_change_action, cardano_blake2b_hash_t* policy_hash);

/**
 * \brief Retrieves the guardrails script hash from a parameter change action.
 *
 * This function retrieves the guardrails script hash (also known as the policy hash) from a given
 * \ref cardano_parameter_change_action_t object. The guardrails script is an optional Plutus script that
 * imposes additional constraints on certain types of governance actions, such as protocol parameter updates
 * and treasury withdrawals. By obtaining the guardrails script hash, you can verify whether the parameter
 * change action references a guardrails script, which may be required for the transaction to be valid.
 *
 * \param[in] parameter_change_action A pointer to an initialized \ref cardano_parameter_change_action_t object from which the guardrails script hash is retrieved.
 *
 * **Return Value:**
 * \return A pointer to the retrieved \ref cardano_blake2b_hash_t object representing the guardrails script hash.
 *         This will be a new reference, and the caller is responsible for releasing it with \ref cardano_blake2b_hash_unref
 *         when it is no longer needed. If the `parameter_change_action` does not have a guardrails script hash set, `NULL` is returned.
 *
 * **Usage Example:**
 * \code{.c}
 * cardano_parameter_change_action_t* parameter_change_action = ...; // Assume initialized
 * cardano_blake2b_hash_t* guardrails_hash = cardano_parameter_change_action_get_policy_hash(parameter_change_action);
 *
 * if (guardrails_hash != NULL)
 * {
 *   char* hash_str = cardano_blake2b_hash_to_string(guardrails_hash);
 *   printf("Guardrails Script Hash: %s\n", hash_str);
 *   // Use the guardrails script hash as needed
 *
 *   // Clean up
 *   cardano_blake2b_hash_unref(&guardrails_hash);
 *   free(hash_str);
 * }
 * else
 * {
 *   printf("No guardrails script hash set for this parameter change action.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_blake2b_hash_t*
cardano_parameter_change_action_get_policy_hash(cardano_parameter_change_action_t* parameter_change_action);

/**
 * \brief Sets the governance action ID in the parameter_change_action.
 *
 * This function updates the governance action ID of a \ref cardano_parameter_change_action_t object.
 * The governance action ID is a \ref cardano_governance_action_id_t object representing the last enacted action of the same type.
 *
 * \param[in,out] parameter_change_action A pointer to an initialized \ref cardano_parameter_change_action_t object to which the governance action ID will be set.
 * \param[in] governance_action_id A pointer to an initialized \ref cardano_governance_action_id_t object representing the last enacted action of the same type. This parameter
 *            can be NULL if the governance action ID is to be unset.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the governance action ID was
 *         successfully set, or an appropriate error code indicating the failure reason, such as \ref CARDANO_ERROR_POINTER_IS_NULL if the
 *         parameter_change_action pointer is NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_parameter_change_action_t* parameter_change_action = ...; // Assume parameter_change_action is already initialized
 * cardano_governance_action_id_t* governance_action_id = cardano_governance_action_id_new(...); // Optionally initialized
 *
 * cardano_error_t result = cardano_parameter_change_action_set_governance_action_id(parameter_change_action, governance_action_id);
 * if (result == CARDANO_SUCCESS)
 * {
 *   // The governance action ID is now set for the parameter_change_action
 * }
 * else
 * {
 *   printf("Failed to set the governance action ID.\n");
 * }
 *
 * // Cleanup the parameter_change_action and optionally the governance action ID
 * cardano_parameter_change_action_unref(&parameter_change_action);
 *
 * if (governance_action_id)
 * {
 *   cardano_governance_action_id_unref(&governance_action_id);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_parameter_change_action_set_governance_action_id(cardano_parameter_change_action_t* parameter_change_action, cardano_governance_action_id_t* governance_action_id);

/**
 * \brief Retrieves the governance action ID from a parameter_change_action.
 *
 * This function retrieves the governance action ID from a given \ref cardano_parameter_change_action_t object. The governance action ID
 * is represented as a \ref cardano_governance_action_id_t object.
 *
 * \param[in] parameter_change_action A pointer to an initialized \ref cardano_parameter_change_action_t object from which the governance action ID is retrieved.
 *
 * \return A pointer to the retrieved \ref cardano_governance_action_id_t object representing the governance action ID.
 *         This will be a new reference, and the caller is responsible for releasing it with \ref cardano_governance_action_id_unref
 *         when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_parameter_change_action_t* parameter_change_action = ...; // Assume initialized
 * cardano_governance_action_id_t* governance_action_id = cardano_parameter_change_action_get_governance_action_id(parameter_change_action);
 *
 * if (governance_action_id != NULL)
 * {
 *   printf("Governance Action ID: %u\n", cardano_governance_action_id_to_uint(governance_action_id));
 *   // Use the governance action ID
 *
 *   // Once done, ensure to clean up and release the governance action ID
 *   cardano_governance_action_id_unref(&governance_action_id);
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_governance_action_id_t*
cardano_parameter_change_action_get_governance_action_id(cardano_parameter_change_action_t* parameter_change_action);

/**
 * \brief Decrements the reference count of a cardano_parameter_change_action_t object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_parameter_change_action_t object
 * by decreasing its reference count. When the reference count reaches zero, the parameter_change_action is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] parameter_change_action A pointer to the pointer of the parameter_change_action object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_parameter_change_action_t* parameter_change_action = cardano_parameter_change_action_new(major, minor);
 *
 * // Perform operations with the parameter_change_action...
 *
 * cardano_parameter_change_action_unref(&parameter_change_action);
 * // At this point, parameter_change_action is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_parameter_change_action_unref, the pointer to the \ref cardano_parameter_change_action_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_parameter_change_action_unref(cardano_parameter_change_action_t** parameter_change_action);

/**
 * \brief Increases the reference count of the cardano_parameter_change_action_t object.
 *
 * This function is used to manually increment the reference count of an cardano_parameter_change_action_t
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_parameter_change_action_unref.
 *
 * \param parameter_change_action A pointer to the cardano_parameter_change_action_t object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming parameter_change_action is a previously created parameter_change_action object
 *
 * cardano_parameter_change_action_ref(parameter_change_action);
 *
 * // Now parameter_change_action can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_parameter_change_action_ref there is a corresponding
 * call to \ref cardano_parameter_change_action_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_parameter_change_action_ref(cardano_parameter_change_action_t* parameter_change_action);

/**
 * \brief Retrieves the current reference count of the cardano_parameter_change_action_t object.
 *
 * This function returns the number of active references to an cardano_parameter_change_action_t object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_parameter_change_action_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param parameter_change_action A pointer to the cardano_parameter_change_action_t object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified cardano_parameter_change_action_t object. If the object
 * is properly managed (i.e., every \ref cardano_parameter_change_action_ref call is matched with a
 * \ref cardano_parameter_change_action_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming parameter_change_action is a previously created parameter_change_action object
 *
 * size_t ref_count = cardano_parameter_change_action_refcount(parameter_change_action);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_parameter_change_action_refcount(const cardano_parameter_change_action_t* parameter_change_action);

/**
 * \brief Sets the last error message for a given cardano_parameter_change_action_t object.
 *
 * Records an error message in the parameter_change_action's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] parameter_change_action A pointer to the \ref cardano_parameter_change_action_t instance whose last error message is
 *                       to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the parameter_change_action's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_parameter_change_action_set_last_error(
  cardano_parameter_change_action_t* parameter_change_action,
  const char*                        message);

/**
 * \brief Retrieves the last error message recorded for a specific parameter_change_action.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_parameter_change_action_set_last_error for the given
 * parameter_change_action. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] parameter_change_action A pointer to the \ref cardano_parameter_change_action_t instance whose last error
 *                   message is to be retrieved. If the parameter_change_action is NULL, the function
 *                   returns a generic error message indicating the null parameter_change_action.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified parameter_change_action. If the parameter_change_action is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_parameter_change_action_set_last_error for the same parameter_change_action, or until
 *       the parameter_change_action is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_parameter_change_action_get_last_error(
  const cardano_parameter_change_action_t* parameter_change_action);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // PARAMETER_CHANGE_ACTION_H