/**
 * \file proposal_procedure.h
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

#ifndef PROPOSAL_PROCEDURE_H
#define PROPOSAL_PROCEDURE_H

/* INCLUDES ******************************************************************/

#include <cardano/address/reward_address.h>
#include <cardano/cbor/cbor_reader.h>
#include <cardano/cbor/cbor_writer.h>
#include <cardano/common/anchor.h>
#include <cardano/error.h>
#include <cardano/export.h>
#include <cardano/proposal_procedures/governance_action_type.h>
#include <cardano/proposal_procedures/update_committee_action.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Governance proposal procedure for the Cardano blockchain, it supports various types of governance actions.
 */
typedef struct cardano_proposal_procedure_t cardano_proposal_procedure_t;

/**
 * \brief Updates one or more updatable protocol parameters, excluding changes to major protocol versions (i.e., "hard forks").
 */
typedef struct cardano_parameter_change_action_t cardano_parameter_change_action_t;

/**
 * \brief Represents the initiation action for a hard fork in the Cardano network.
 */
typedef struct cardano_hard_fork_initiation_action_t cardano_hard_fork_initiation_action_t;

/**
 * \brief Withdraws funds from the treasury.
 */
typedef struct cardano_treasury_withdrawals_action_t cardano_treasury_withdrawals_action_t;

/**
 * \brief Propose a state of no-confidence in the current constitutional committee.
 * Allows Ada holders to challenge the authority granted to the existing committee.
 */
typedef struct cardano_no_confidence_action_t cardano_no_confidence_action_t;

/**
 * \brief Modifies the composition of the constitutional committee, its signature threshold, or its terms of operation.
 */
typedef struct cardano_update_committee_action_t cardano_update_committee_action_t;

/**
 * \brief Changes or amendments the Constitution.
 */
typedef struct cardano_new_constitution_action_t cardano_new_constitution_action_t;

/**
 * \brief Represents an action that has no direct effect on the blockchain,
 * but serves as an on-chain record or informative notice.
 */
typedef struct cardano_info_action_t cardano_info_action_t;

/**
 * \brief Creates a new proposal procedure for a parameter change action.
 *
 * This function allocates and initializes a new instance of a \ref cardano_proposal_procedure_t object,
 * which is designed to propose changes to the protocol parameters within the Cardano network.
 *
 * \param[in] deposit The deposit required to submit the proposal.
 * \param[in] reward_address A pointer to a \ref cardano_reward_address_t object associated.
 * \param[in] anchor A pointer to a \ref cardano_anchor_t object representing additional information related to the proposal, can be NULL if not applicable.
 * \param[in] parameter_change_action A pointer to an initialized \ref cardano_parameter_change_action_t object detailing the proposed changes to protocol parameters.
 * \param[out] proposal On successful initialization, this will point to a newly created \ref cardano_proposal_procedure_t object.
 *             This object represents a "strong reference" to the proposal procedure, meaning that it is fully initialized and ready for use.
 *             The caller is responsible for managing the lifecycle of this object,
 *             specifically, once the proposal procedure is no longer needed, the caller must release it
 *             by calling \ref cardano_proposal_procedure_unref.
 *
 * \return \c cardano_error_t indicating the outcome of the operation. Returns \c CARDANO_SUCCESS if the proposal procedure was
 *         successfully created, or an appropriate error code indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * uint64_t deposit = 1000000; // Example deposit amount
 * cardano_reward_address_t* reward_address = cardano_reward_address_new(...); // Assume initialized
 * cardano_anchor_t* anchor = cardano_anchor_new(...); // Optionally initialized
 * cardano_parameter_change_action_t* parameter_change_action = cardano_parameter_change_action_new(...); // Assume initialized
 * cardano_proposal_procedure_t* proposal = NULL;
 * cardano_error_t result = cardano_proposal_procedure_new_parameter_change_action(deposit, reward_address, anchor, parameter_change_action, &proposal);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the proposal procedure
 *   // Free resources when done
 *   cardano_proposal_procedure_unref(&proposal);
 * }
 * else
 * {
 *   printf("Failed to create the proposal procedure: %s\n", cardano_error_to_string(result));
 * }
 *
 * // Cleanup resources used for setup
 * cardano_reward_address_unref(&reward_address);
 * cardano_anchor_unref(&anchor);
 * cardano_parameter_change_action_unref(&parameter_change_action);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_proposal_procedure_new_parameter_change_action(
  uint64_t                           deposit,
  cardano_reward_address_t*          reward_address,
  cardano_anchor_t*                  anchor,
  cardano_parameter_change_action_t* parameter_change_action,
  cardano_proposal_procedure_t**     proposal);

/**
 * \brief Creates a new proposal procedure for a hard fork initiation action.
 *
 * This function allocates and initializes a new instance of a \ref cardano_proposal_procedure_t object.
 *
 * \param[in] deposit The deposit required to submit the proposal.
 * \param[in] reward_address A pointer to a \ref cardano_reward_address_t object associated.
 * \param[in] anchor A pointer to a \ref cardano_anchor_t object representing additional information related to the proposal, can be NULL if not applicable.
 * \param[in] hard_fork_initiation_action A pointer to an initialized \ref cardano_hard_fork_initiation_action_t object detailing the proposed hard fork.
 * \param[out] proposal On successful initialization, this will point to a newly created \ref cardano_proposal_procedure_t object.
 *             This object represents a "strong reference" to the proposal procedure, meaning that it is fully initialized and ready for use.
 *             The caller is responsible for managing the lifecycle of this object,
 *             specifically, once the proposal procedure is no longer needed, the caller must release it
 *             by calling \ref cardano_proposal_procedure_unref.
 *
 * \return \c cardano_error_t indicating the outcome of the operation. Returns \c CARDANO_SUCCESS if the proposal procedure was
 *         successfully created, or an appropriate error code indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * uint64_t deposit = 1000000; // Example deposit amount
 * cardano_reward_address_t* reward_address = cardano_reward_address_new(...); // Assume reward address is already initialized
 * cardano_anchor_t* anchor = cardano_anchor_new(...); // Optionally initialized
 * cardano_hard_fork_initiation_action_t* hard_fork_initiation_action = cardano_hard_fork_initiation_action_new(...); // Assume initialized
 * cardano_proposal_procedure_t* proposal = NULL;
 * cardano_error_t result = cardano_proposal_procedure_new_hard_fork_initiation_action(deposit, reward_address, anchor, hard_fork_initiation_action, &proposal);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the proposal procedure for hard fork initiation
 *   // Free resources when done
 *   cardano_proposal_procedure_unref(&proposal);
 * }
 * else
 * {
 *   printf("Failed to create the hard fork initiation proposal: %s\n", cardano_error_to_string(result));
 * }
 *
 * // Cleanup resources used for setup
 * cardano_reward_address_unref(&reward_address);
 * cardano_anchor_unref(&anchor);
 * cardano_hard_fork_initiation_action_unref(&hard_fork_initiation_action);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_proposal_procedure_new_hard_fork_initiation_action(
  uint64_t                               deposit,
  cardano_reward_address_t*              reward_address,
  cardano_anchor_t*                      anchor,
  cardano_hard_fork_initiation_action_t* hard_fork_initiation_action,
  cardano_proposal_procedure_t**         proposal);

/**
 * \brief Creates a new proposal procedure for a treasury withdrawals action.
 *
 * This function allocates and initializes a new instance of a \ref cardano_proposal_procedure_t object.
 *
 * \param[in] deposit The deposit required to submit the proposal.
 * \param[in] reward_address A pointer to a \ref cardano_reward_address_t object associated.
 * \param[in] anchor A pointer to a \ref cardano_anchor_t object representing additional information related to the proposal, can be NULL if not applicable.
 * \param[in] treasury_withdrawals_action A pointer to an initialized \ref cardano_treasury_withdrawals_action_t object detailing the proposed hard fork.
 * \param[out] proposal On successful initialization, this will point to a newly created \ref cardano_proposal_procedure_t object.
 *             This object represents a "strong reference" to the proposal procedure, meaning that it is fully initialized and ready for use.
 *             The caller is responsible for managing the lifecycle of this object,
 *             specifically, once the proposal procedure is no longer needed, the caller must release it
 *             by calling \ref cardano_proposal_procedure_unref.
 *
 * \return \c cardano_error_t indicating the outcome of the operation. Returns \c CARDANO_SUCCESS if the proposal procedure was
 *         successfully created, or an appropriate error code indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * uint64_t deposit = 1000000; // Example deposit amount
 * cardano_reward_address_t* reward_address = cardano_reward_address_new(...); // Assume reward address is already initialized
 * cardano_anchor_t* anchor = cardano_anchor_new(...); // Optionally initialized
 * cardano_treasury_withdrawals_action_t* treasury_withdrawals_action = cardano_treasury_withdrawals_action_new(...); // Assume initialized
 * cardano_proposal_procedure_t* proposal = NULL;
 * cardano_error_t result = cardano_proposal_procedure_new_treasury_withdrawals_action(deposit, reward_address, anchor, treasury_withdrawals_action, &proposal);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the proposal procedure for treasury withdrawals
 *   // Free resources when done
 *   cardano_proposal_procedure_unref(&proposal);
 * }
 * else
 * {
 *   printf("Failed to create the treasury withdrawals proposal: %s\n", cardano_error_to_string(result));
 * }
 *
 * // Cleanup resources used for setup
 * cardano_reward_address_unref(&reward_address);
 * cardano_anchor_unref(&anchor);
 * cardano_treasury_withdrawals_action_unref(&treasury_withdrawals_action);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_proposal_procedure_new_treasury_withdrawals_action(
  uint64_t                               deposit,
  cardano_reward_address_t*              reward_address,
  cardano_anchor_t*                      anchor,
  cardano_treasury_withdrawals_action_t* treasury_withdrawals_action,
  cardano_proposal_procedure_t**         proposal);

/**
 * \brief Creates a new proposal procedure for a no confidence action.
 *
 * This function allocates and initializes a new instance of a \ref cardano_proposal_procedure_t object.
 *
 * \param[in] deposit The deposit required to submit the proposal.
 * \param[in] reward_address A pointer to a \ref cardano_reward_address_t object associated.
 * \param[in] anchor A pointer to a \ref cardano_anchor_t object representing additional information related to the proposal, can be NULL if not applicable.
 * \param[in] no_confidence_action A pointer to an initialized \ref cardano_no_confidence_action_t object detailing the proposed hard fork.
 * \param[out] proposal On successful initialization, this will point to a newly created \ref cardano_proposal_procedure_t object.
 *             This object represents a "strong reference" to the proposal procedure, meaning that it is fully initialized and ready for use.
 *             The caller is responsible for managing the lifecycle of this object,
 *             specifically, once the proposal procedure is no longer needed, the caller must release it
 *             by calling \ref cardano_proposal_procedure_unref.
 *
 * \return \c cardano_error_t indicating the outcome of the operation. Returns \c CARDANO_SUCCESS if the proposal procedure was
 *         successfully created, or an appropriate error code indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * uint64_t deposit = 1000000; // Example deposit amount
 * cardano_reward_address_t* reward_address = cardano_reward_address_new(...); // Assume reward address is already initialized
 * cardano_anchor_t* anchor = cardano_anchor_new(...); // Optionally initialized
 * cardano_no_confidence_action_t* no_confidence_action = cardano_no_confidence_action_new(...); // Assume initialized
 * cardano_proposal_procedure_t* proposal = NULL;
 * cardano_error_t result = cardano_proposal_procedure_new_no_confidence_action(deposit, reward_address, anchor, no_confidence_action, &proposal);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the proposal procedure for no confidence
 *   // Free resources when done
 *   cardano_proposal_procedure_unref(&proposal);
 * }
 * else
 * {
 *   printf("Failed to create the no confidence proposal: %s\n", cardano_error_to_string(result));
 * }
 *
 * // Cleanup resources used for setup
 * cardano_reward_address_unref(&reward_address);
 * cardano_anchor_unref(&anchor);
 * cardano_no_confidence_action_unref(&no_confidence_action);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_proposal_procedure_new_no_confidence_action(
  uint64_t                        deposit,
  cardano_reward_address_t*       reward_address,
  cardano_anchor_t*               anchor,
  cardano_no_confidence_action_t* no_confidence_action,
  cardano_proposal_procedure_t**  proposal);

/**
 * \brief Creates a new proposal procedure for a update committee action.
 *
 * This function allocates and initializes a new instance of a \ref cardano_proposal_procedure_t object.
 *
 * \param[in] deposit The deposit required to submit the proposal.
 * \param[in] reward_address A pointer to a \ref cardano_reward_address_t object associated.
 * \param[in] anchor A pointer to a \ref cardano_anchor_t object representing additional information related to the proposal, can be NULL if not applicable.
 * \param[in] update_committee_action A pointer to an initialized \ref cardano_update_committee_action_t object detailing the proposed hard fork.
 * \param[out] proposal On successful initialization, this will point to a newly created \ref cardano_proposal_procedure_t object.
 *             This object represents a "strong reference" to the proposal procedure, meaning that it is fully initialized and ready for use.
 *             The caller is responsible for managing the lifecycle of this object,
 *             specifically, once the proposal procedure is no longer needed, the caller must release it
 *             by calling \ref cardano_proposal_procedure_unref.
 *
 * \return \c cardano_error_t indicating the outcome of the operation. Returns \c CARDANO_SUCCESS if the proposal procedure was
 *         successfully created, or an appropriate error code indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * uint64_t deposit = 1000000; // Example deposit amount
 * cardano_reward_address_t* reward_address = cardano_reward_address_new(...); // Assume reward address is already initialized
 * cardano_anchor_t* anchor = cardano_anchor_new(...); // Optionally initialized
 * cardano_update_committee_action_t* update_committee_action = cardano_update_committee_action_new(...); // Assume initialized
 * cardano_proposal_procedure_t* proposal = NULL;
 * cardano_error_t result = cardano_proposal_procedure_new_update_committee_action(deposit, reward_address, anchor, update_committee_action, &proposal);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the proposal procedure for update committee
 *   // Free resources when done
 *   cardano_proposal_procedure_unref(&proposal);
 * }
 * else
 * {
 *   printf("Failed to create the update committee proposal: %s\n", cardano_error_to_string(result));
 * }
 *
 * // Cleanup resources used for setup
 * cardano_reward_address_unref(&reward_address);
 * cardano_anchor_unref(&anchor);
 * cardano_update_committee_action_unref(&update_committee_action);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_proposal_procedure_new_update_committee_action(
  uint64_t                           deposit,
  cardano_reward_address_t*          reward_address,
  cardano_anchor_t*                  anchor,
  cardano_update_committee_action_t* update_committee_action,
  cardano_proposal_procedure_t**     proposal);

/**
 * \brief Creates a new proposal procedure for a new constitution action.
 *
 * This function allocates and initializes a new instance of a \ref cardano_proposal_procedure_t object.
 *
 * \param[in] deposit The deposit required to submit the proposal.
 * \param[in] reward_address A pointer to a \ref cardano_reward_address_t object associated.
 * \param[in] anchor A pointer to a \ref cardano_anchor_t object representing additional information related to the proposal, can be NULL if not applicable.
 * \param[in] new_constitution_action A pointer to an initialized \ref cardano_new_constitution_action_t object detailing the proposed hard fork.
 * \param[out] proposal On successful initialization, this will point to a newly created \ref cardano_proposal_procedure_t object.
 *             This object represents a "strong reference" to the proposal procedure, meaning that it is fully initialized and ready for use.
 *             The caller is responsible for managing the lifecycle of this object,
 *             specifically, once the proposal procedure is no longer needed, the caller must release it
 *             by calling \ref cardano_proposal_procedure_unref.
 *
 * \return \c cardano_error_t indicating the outcome of the operation. Returns \c CARDANO_SUCCESS if the proposal procedure was
 *         successfully created, or an appropriate error code indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * uint64_t deposit = 1000000; // Example deposit amount
 * cardano_reward_address_t* reward_address = cardano_reward_address_new(...); // Assume reward address is already initialized
 * cardano_anchor_t* anchor = cardano_anchor_new(...); // Optionally initialized
 * cardano_new_constitution_action_t* new_constitution_action = cardano_new_constitution_action_new(...); // Assume initialized
 * cardano_proposal_procedure_t* proposal = NULL;
 * cardano_error_t result = cardano_proposal_procedure_new_new_constitution_action(deposit, reward_address, anchor, new_constitution_action, &proposal);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the proposal procedure for new constitution
 *   // Free resources when done
 *   cardano_proposal_procedure_unref(&proposal);
 * }
 * else
 * {
 *   printf("Failed to create the new constitution proposal: %s\n", cardano_error_to_string(result));
 * }
 *
 * // Cleanup resources used for setup
 * cardano_reward_address_unref(&reward_address);
 * cardano_anchor_unref(&anchor);
 * cardano_new_constitution_action_unref(&new_constitution_action);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_proposal_procedure_new_constitution_action(
  uint64_t                           deposit,
  cardano_reward_address_t*          reward_address,
  cardano_anchor_t*                  anchor,
  cardano_new_constitution_action_t* new_constitution_action,
  cardano_proposal_procedure_t**     proposal);

/**
 * \brief Creates a new proposal procedure for a info action.
 *
 * This function allocates and initializes a new instance of a \ref cardano_proposal_procedure_t object.
 *
 * \param[in] deposit The deposit required to submit the proposal.
 * \param[in] reward_address A pointer to a \ref cardano_reward_address_t object associated.
 * \param[in] anchor A pointer to a \ref cardano_anchor_t object representing additional information related to the proposal, can be NULL if not applicable.
 * \param[in] info_action A pointer to an initialized \ref cardano_info_action_t object detailing the proposed hard fork.
 * \param[out] proposal On successful initialization, this will point to a newly created \ref cardano_proposal_procedure_t object.
 *             This object represents a "strong reference" to the proposal procedure, meaning that it is fully initialized and ready for use.
 *             The caller is responsible for managing the lifecycle of this object,
 *             specifically, once the proposal procedure is no longer needed, the caller must release it
 *             by calling \ref cardano_proposal_procedure_unref.
 *
 * \return \c cardano_error_t indicating the outcome of the operation. Returns \c CARDANO_SUCCESS if the proposal procedure was
 *         successfully created, or an appropriate error code indicating the failure reason.
 *
 * Usage Example:
 * \code{.c}
 * uint64_t deposit = 1000000; // Example deposit amount
 * cardano_reward_address_t* reward_address = cardano_reward_address_new(...); // Assume reward address is already initialized
 * cardano_anchor_t* anchor = cardano_anchor_new(...); // Optionally initialized
 * cardano_info_action_t* info_action = cardano_info_action_new(...); // Assume initialized
 * cardano_proposal_procedure_t* proposal = NULL;
 * cardano_error_t result = cardano_proposal_procedure_new_info_action(deposit, reward_address, anchor, info_action, &proposal);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the proposal procedure for info
 *   // Free resources when done
 *   cardano_proposal_procedure_unref(&proposal);
 * }
 * else
 * {
 *   printf("Failed to create the info proposal: %s\n", cardano_error_to_string(result));
 * }
 *
 * // Cleanup resources used for setup
 * cardano_reward_address_unref(&reward_address);
 * cardano_anchor_unref(&anchor);
 * cardano_info_action_unref(&info_action);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_proposal_procedure_new_info_action(
  uint64_t                       deposit,
  cardano_reward_address_t*      reward_address,
  cardano_anchor_t*              anchor,
  cardano_info_action_t*         info_action,
  cardano_proposal_procedure_t** proposal);

/**
 * \brief Creates a \ref cardano_proposal_procedure_t from a CBOR reader.
 *
 * This function parses CBOR data using a provided \ref cardano_cbor_reader_t and constructs a \ref cardano_proposal_procedure_t object.
 * It assumes that the CBOR reader is set up correctly and that the CBOR data corresponds to the structure expected for a auth_committee_hot.
 *
 * \param[in] reader A pointer to an initialized \ref cardano_cbor_reader_t that is ready to read the CBOR-encoded data.
 * \param[out] auth_committee_hot A pointer to a pointer of \ref cardano_proposal_procedure_t that will be set to the address
 *                        of the newly created auth_committee_hot object upon successful decoding.
 *
 * \return A \ref cardano_error_t value indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the object was successfully created, or an appropriate error code if an error occurred.
 *
 * \note If the function fails, the last error can be retrieved by calling \ref cardano_cbor_reader_get_last_error with the reader.
 *       The caller is responsible for freeing the created \ref cardano_proposal_procedure_t object by calling
 *       \ref cardano_proposal_procedure_unref when it is no longer needed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_cbor_reader_t* reader = cardano_cbor_reader_new(cbor_data, data_size);
 * cardano_proposal_procedure_t* auth_committee_hot = NULL;
 *
 * cardano_error_t result = cardano_proposal_procedure_from_cbor(reader, &auth_committee_hot);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Use the auth_committee_hot
 *
 *   // Once done, ensure to clean up and release the auth_committee_hot
 *   cardano_proposal_procedure_unref(&auth_committee_hot);
 * }
 * else
 * {
 *   const char* error = cardano_cbor_reader_get_last_error(reader);
 *   printf("Failed to decode auth_committee_hot: %s\n", error);
 * }
 *
 * cardano_cbor_reader_unref(&reader); // Cleanup the CBOR reader
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_proposal_procedure_from_cbor(cardano_cbor_reader_t* reader, cardano_proposal_procedure_t** auth_committee_hot);

/**
 * \brief Serializes the proposal_procedure into CBOR format using a CBOR writer.
 *
 * This function serializes the given \ref cardano_proposal_procedure_t object using a \ref cardano_cbor_writer_t.
 *
 * \param[in] auth_committee_hot A constant pointer to the \ref cardano_proposal_procedure_t object that is to be serialized.
 * \param[out] writer A pointer to a \ref cardano_cbor_writer_t where the CBOR serialized data will be written.
 *                    The writer must already be initialized and ready to accept the data.
 *
 * \return Returns \ref CARDANO_SUCCESS if the serialization is successful. If the \p auth_committee_hot or \p writer
 *         is NULL, returns \ref CARDANO_ERROR_POINTER_IS_NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_proposal_procedure_t* auth_committee_hot = ...;
 * cardano_cbor_writer_t* writer = cardano_cbor_writer_new();
 *
 * if (writer)
 * {
 *   cardano_error_t result = cardano_proposal_procedure_to_cbor(auth_committee_hot, writer);
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
 * cardano_proposal_procedure_unref(&auth_committee_hot);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t cardano_proposal_procedure_to_cbor(
  const cardano_proposal_procedure_t* auth_committee_hot,
  cardano_cbor_writer_t*              writer);

/**
 * \brief Retrieves the type of governance action associated with a proposal procedure.
 *
 * This function extracts the type of governance action, represented by a \ref cardano_governance_action_id_t object,
 * from a given \ref cardano_proposal_procedure_t object.
 *
 * \param[in] proposal A constant pointer to the \ref cardano_proposal_procedure_t object from which
 *                     the action type is to be retrieved. The object must not be NULL.
 * \param[out] type Pointer to a variable where the action type will be stored. This variable will
 *                   be set to the value from the \ref cardano_governance_action_id_t enumeration.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the action type was
 *         successfully retrieved, or an appropriate error code if the input is NULL or any other error occurs.
 *
 * Usage Example:
 * \code{.c}
 * cardano_proposal_procedure_t* proposal = cardano_proposal_procedure_new(...);
 * cardano_governance_action_id_t type;
 * cardano_error_t result = cardano_proposal_procedure_get_action_type(proposal, &type);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Action Type: %d\n", type);
 *   // Handle the action type accordingly
 * }
 *
 * // Clean up the proposal object once done
 * cardano_proposal_procedure_unref(&proposal);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_proposal_procedure_get_action_type(const cardano_proposal_procedure_t* proposal, cardano_governance_action_type_t* type);

/**
 * \brief Converts a proposal procedure into a parameter change action if applicable.
 *
 * This function attempts to convert a given \ref cardano_proposal_procedure_t object into a \ref cardano_parameter_change_action_t object.
 * This is applicable only if the proposal procedure encapsulates a parameter change action.
 *
 * \param[in] proposal A pointer to the \ref cardano_proposal_procedure_t object that potentially contains a parameter change action.
 * \param[out] parameter_change_action On successful conversion, this will point to a newly created
 *             \ref cardano_parameter_change_action_t object representing the parameter change action.
 *             The caller is responsible for managing the lifecycle of this object, including releasing it
 *             by calling \ref cardano_parameter_change_action_unref when it is no longer needed.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the conversion was
 *         successful, or an appropriate error code if the proposal does not contain a parameter change action or if any other
 *         error occurs.
 *
 * Usage Example:
 * \code{.c}
 * cardano_proposal_procedure_t* proposal = cardano_proposal_procedure_new(...);
 * cardano_parameter_change_action_t* parameter_change_action = NULL;
 * cardano_error_t result = cardano_proposal_procedure_to_parameter_change_action(proposal, &parameter_change_action);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Successfully retrieved the parameter change action
 *   // Use the parameter_change_action as needed
 *
 *   // Clean up the parameter change action when done
 *   cardano_parameter_change_action_unref(&parameter_change_action);
 * }
 * else
 * {
 *   printf("Failed to convert to parameter change action: %s\n", cardano_error_to_string(result));
 * }
 *
 * // Clean up the proposal object after use
 * cardano_proposal_procedure_unref(&proposal);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_proposal_procedure_to_parameter_change_action(
  cardano_proposal_procedure_t*       proposal,
  cardano_parameter_change_action_t** parameter_change_action);

/**
 * \brief Converts a proposal procedure into a hard fork initiation action if applicable.
 *
 * This function attempts to convert a given \ref cardano_proposal_procedure_t object into a \ref cardano_hard_fork_initiation_action_t object.
 * This is applicable only if the proposal procedure encapsulates a hard fork initiation action.
 *
 * \param[in] proposal A pointer to the \ref cardano_proposal_procedure_t object that potentially contains a hard fork initiation action.
 * \param[out] hard_fork_initiation_action On successful conversion, this will point to a newly created
 *             \ref cardano_hard_fork_initiation_action_t object representing the hard fork initiation action.
 *             The caller is responsible for managing the lifecycle of this object, including releasing it
 *             by calling \ref cardano_hard_fork_initiation_action_unref when it is no longer needed.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the conversion was
 *         successful, or an appropriate error code if the proposal does not contain a hard fork initiation action or if any other
 *         error occurs.
 *
 * Usage Example:
 * \code{.c}
 * cardano_proposal_procedure_t* proposal = cardano_proposal_procedure_new(...);
 * cardano_hard_fork_initiation_action_t* hard_fork_initiation_action = NULL;
 * cardano_error_t result = cardano_proposal_procedure_to_hard_fork_initiation_action(proposal, &hard_fork_initiation_action);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Successfully retrieved the hard fork initiation action
 *   // Use the hard_fork_initiation_action as needed
 *
 *   // Clean up the hard fork initiation action when done
 *   cardano_hard_fork_initiation_action_unref(&hard_fork_initiation_action);
 * }
 * else
 * {
 *   printf("Failed to convert to hard fork initiation action: %s\n", cardano_error_to_string(result));
 * }
 *
 * // Clean up the proposal object after use
 * cardano_proposal_procedure_unref(&proposal);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_proposal_procedure_to_hard_fork_initiation_action(
  cardano_proposal_procedure_t*           proposal,
  cardano_hard_fork_initiation_action_t** hard_fork_initiation_action);

/**
 * \brief Converts a proposal procedure into a treasury withdrawals action if applicable.
 *
 * This function attempts to convert a given \ref cardano_proposal_procedure_t object into a \ref cardano_treasury_withdrawals_action_t object.
 * This is applicable only if the proposal procedure encapsulates a treasury withdrawals action.
 *
 * \param[in] proposal A pointer to the \ref cardano_proposal_procedure_t object that potentially contains a treasury withdrawals action.
 * \param[out] treasury_withdrawals_action On successful conversion, this will point to a newly created
 *             \ref cardano_treasury_withdrawals_action_t object representing the treasury withdrawals action.
 *             The caller is responsible for managing the lifecycle of this object, including releasing it
 *             by calling \ref cardano_treasury_withdrawals_action_unref when it is no longer needed.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the conversion was
 *         successful, or an appropriate error code if the proposal does not contain a treasury withdrawals action or if any other
 *         error occurs.
 *
 * Usage Example:
 * \code{.c}
 * cardano_proposal_procedure_t* proposal = cardano_proposal_procedure_new(...);
 * cardano_treasury_withdrawals_action_t* treasury_withdrawals_action = NULL;
 * cardano_error_t result = cardano_proposal_procedure_to_treasury_withdrawals_action(proposal, &treasury_withdrawals_action);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Successfully retrieved the treasury withdrawals action
 *   // Use the treasury_withdrawals_action as needed
 *
 *   // Clean up the treasury withdrawals action when done
 *   cardano_treasury_withdrawals_action_unref(&treasury_withdrawals_action);
 * }
 * else
 * {
 *   printf("Failed to convert to treasury withdrawals action: %s\n", cardano_error_to_string(result));
 * }
 *
 * // Clean up the proposal object after use
 * cardano_proposal_procedure_unref(&proposal);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_proposal_procedure_to_treasury_withdrawals_action(
  cardano_proposal_procedure_t*           proposal,
  cardano_treasury_withdrawals_action_t** treasury_withdrawals_action);

/**
 * \brief Converts a proposal procedure into a no confidence action if applicable.
 *
 * This function attempts to convert a given \ref cardano_proposal_procedure_t object into a \ref cardano_no_confidence_action_t object.
 * This is applicable only if the proposal procedure encapsulates a no confidence action.
 *
 * \param[in] proposal A pointer to the \ref cardano_proposal_procedure_t object that potentially contains a no confidence action.
 * \param[out] no_confidence_action On successful conversion, this will point to a newly created
 *             \ref cardano_no_confidence_action_t object representing the no confidence action.
 *             The caller is responsible for managing the lifecycle of this object, including releasing it
 *             by calling \ref cardano_no_confidence_action_unref when it is no longer needed.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the conversion was
 *         successful, or an appropriate error code if the proposal does not contain a no confidence action or if any other
 *         error occurs.
 *
 * Usage Example:
 * \code{.c}
 * cardano_proposal_procedure_t* proposal = cardano_proposal_procedure_new(...);
 * cardano_no_confidence_action_t* no_confidence_action = NULL;
 * cardano_error_t result = cardano_proposal_procedure_to_no_confidence_action(proposal, &no_confidence_action);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Successfully retrieved the no confidence action
 *   // Use the no_confidence_action as needed
 *
 *   // Clean up the no confidence action when done
 *   cardano_no_confidence_action_unref(&no_confidence_action);
 * }
 * else
 * {
 *   printf("Failed to convert to no confidence action: %s\n", cardano_error_to_string(result));
 * }
 *
 * // Clean up the proposal object after use
 * cardano_proposal_procedure_unref(&proposal);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_proposal_procedure_to_no_confidence_action(
  cardano_proposal_procedure_t*    proposal,
  cardano_no_confidence_action_t** no_confidence_action);

/**
 * \brief Converts a proposal procedure into an update committee action if applicable.
 *
 * This function attempts to convert a given \ref cardano_proposal_procedure_t object into a \ref cardano_update_committee_action_t object.
 * This is applicable only if the proposal procedure encapsulates an update committee action.
 *
 * \param[in] proposal A pointer to the \ref cardano_proposal_procedure_t object that potentially contains an update committee action.
 * \param[out] update_committee_action On successful conversion, this will point to a newly created
 *             \ref cardano_update_committee_action_t object representing the update committee action.
 *             The caller is responsible for managing the lifecycle of this object, including releasing it
 *             by calling \ref cardano_update_committee_action_unref when it is no longer needed.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the conversion was
 *         successful, or an appropriate error code if the proposal does not contain an update committee action or if any other
 *         error occurs.
 *
 * Usage Example:
 * \code{.c}
 * cardano_proposal_procedure_t* proposal = cardano_proposal_procedure_new(...);
 * cardano_update_committee_action_t* update_committee_action = NULL;
 * cardano_error_t result = cardano_proposal_procedure_to_update_committee_action(proposal, &update_committee_action);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Successfully retrieved the update committee action
 *   // Use the update_committee_action as needed
 *
 *   // Clean up the update committee action when done
 *   cardano_update_committee_action_unref(&update_committee_action);
 * }
 * else
 * {
 *   printf("Failed to convert to update committee action: %s\n", cardano_error_to_string(result));
 * }
 *
 * // Clean up the proposal object after use
 * cardano_proposal_procedure_unref(&proposal);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_proposal_procedure_to_update_committee_action(
  cardano_proposal_procedure_t*       proposal,
  cardano_update_committee_action_t** update_committee_action);

/**
 * \brief Converts a proposal procedure into a new constitution action if applicable.
 *
 * This function attempts to convert a given \ref cardano_proposal_procedure_t object into a \ref cardano_new_constitution_action_t object.
 * This is applicable only if the proposal procedure encapsulates a new constitution action.
 *
 * \param[in] proposal A pointer to the \ref cardano_proposal_procedure_t object that potentially contains a new constitution action.
 * \param[out] constitution_action On successful conversion, this will point to a newly created
 *             \ref cardano_new_constitution_action_t object representing the new constitution action.
 *             The caller is responsible for managing the lifecycle of this object, including releasing it
 *             by calling \ref cardano_new_constitution_action_unref when it is no longer needed.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the conversion was
 *         successful, or an appropriate error code if the proposal does not contain a new constitution action or if any other
 *         error occurs.
 *
 * Usage Example:
 * \code{.c}
 * cardano_proposal_procedure_t* proposal = cardano_proposal_procedure_new(...);
 * cardano_new_constitution_action_t* constitution_action = NULL;
 * cardano_error_t result = cardano_proposal_procedure_to_constitution_action(proposal, &constitution_action);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Successfully retrieved the new constitution action
 *   // Use the constitution_action as needed
 *
 *   // Clean up the new constitution action when done
 *   cardano_new_constitution_action_unref(&constitution_action);
 * }
 * else
 * {
 *   printf("Failed to convert to new constitution action: %s\n", cardano_error_to_string(result));
 * }
 *
 * // Clean up the proposal object after use
 * cardano_proposal_procedure_unref(&proposal);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_proposal_procedure_to_constitution_action(
  cardano_proposal_procedure_t*       proposal,
  cardano_new_constitution_action_t** constitution_action);

/**
 * \brief Converts a proposal procedure into an info action if applicable.
 *
 * This function attempts to convert a given \ref cardano_proposal_procedure_t object into a \ref cardano_info_action_t object.
 * This is applicable only if the proposal procedure encapsulates an info action, which is intended for recording informative
 * notices on the blockchain without directly affecting its operational state.
 *
 * \param[in] proposal A pointer to the \ref cardano_proposal_procedure_t object that potentially contains an info action.
 * \param[out] info_action On successful conversion, this will point to a newly created
 *             \ref cardano_info_action_t object representing the info action. The caller is responsible for managing the lifecycle
 *             of this object, including releasing it by calling \ref cardano_info_action_unref when it is no longer needed.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the conversion was
 *         successful, or an appropriate error code if the proposal does not contain an info action or if any other
 *         error occurs.
 *
 * Usage Example:
 * \code{.c}
 * cardano_proposal_procedure_t* proposal = cardano_proposal_procedure_new(...);
 * cardano_info_action_t* info_action = NULL;
 * cardano_error_t result = cardano_proposal_procedure_to_info_action(proposal, &info_action);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   // Successfully retrieved the info action
 *   // Use the info_action as needed
 *
 *   // Clean up the info action when done
 *   cardano_info_action_unref(&info_action);
 * }
 * else
 * {
 *   printf("Failed to convert to info action: %s\n", cardano_error_to_string(result));
 * }
 *
 * // Clean up the proposal object after use
 * cardano_proposal_procedure_unref(&proposal);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_proposal_procedure_to_info_action(
  cardano_proposal_procedure_t* proposal,
  cardano_info_action_t**       info_action);

/**
 * \brief Sets the anchor in the proposal procedure.
 *
 * This function updates the anchor of a \ref cardano_proposal_procedure_t object. The anchor is used to link to the off-chain content of the proposal_procedure.
 *
 * \param[in,out] proposal_procedure A pointer to an initialized \ref cardano_proposal_procedure_t object to which the anchor will be set.
 * \param[in] anchor A pointer to an initialized \ref cardano_anchor_t object representing the new anchor.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the anchor was
 *         successfully set, or an appropriate error code indicating the failure reason, such as \ref CARDANO_ERROR_POINTER_IS_NULL if any of the
 *         input pointers are NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_proposal_procedure_t* proposal_procedure = ...; // Assume proposal_procedure is already initialized
 * cardano_anchor_t* anchor = cardano_anchor_new(...); // Assume anchor is already initialized
 *
 * cardano_error_t result = cardano_proposal_procedure_set_anchor(proposal_procedure, anchor);
 * if (result == CARDANO_SUCCESS)
 * {
 *   // The anchor is now set for the proposal_procedure
 * }
 * else
 * {
 *   printf("Failed to set the anchor.\n");
 * }
 * // Clean up the proposal_procedure and anchor after use
 * cardano_proposal_procedure_unref(&proposal_procedure);
 * cardano_anchor_unref(&anchor);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_proposal_procedure_set_anchor(cardano_proposal_procedure_t* proposal_procedure, cardano_anchor_t* anchor);

/**
 * \brief Gets the anchor from a proposal_procedure.
 *
 * This function retrieves the anchor from a given \ref cardano_proposal_procedure_t object. The anchor links to the off-chain content of the proposal_procedure.
 *
 * \param[in] proposal_procedure A pointer to an initialized \ref cardano_proposal_procedure_t object from which the anchor is retrieved.
 *
 * \return A pointer to the retrieved \ref cardano_anchor_t object representing the anchor. This will be a new reference,
 *         and the caller is responsible for releasing it with \ref cardano_anchor_unref when it is no longer needed.
 *         If the proposal_procedure does not have an anchor set, NULL is returned.
 *
 * Usage Example:
 * \code{.c}
 * cardano_proposal_procedure_t* proposal_procedure = ...; // Assume initialized
 * cardano_anchor_t* anchor = cardano_proposal_procedure_get_anchor(proposal_procedure);
 *
 * if (anchor != NULL)
 * {
 *   // Use the anchor
 *
 *   // Once done, ensure to clean up and release the anchor
 *   cardano_anchor_unref(&anchor);
 * }
 * else
 * {
 *   printf("ERROR: No anchor set for this proposal_procedure.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_anchor_t*
cardano_proposal_procedure_get_anchor(cardano_proposal_procedure_t* proposal_procedure);

/**
 * \brief Sets the reward address in the proposal procedure.
 *
 * This function updates the reward_address of a \ref cardano_proposal_procedure_t object.
 *
 * \param[in,out] proposal_procedure A pointer to an initialized \ref cardano_proposal_procedure_t object to which the reward address will be set.
 * \param[in] reward_address A pointer to an initialized \ref cardano_reward_address_t object representing the new reward address.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS if the reward_address was
 *         successfully set, or an appropriate error code indicating the failure reason, such as \ref CARDANO_ERROR_POINTER_IS_NULL if any of the
 *         input pointers are NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_proposal_procedure_t* proposal_procedure = ...; // Assume proposal_procedure is already initialized
 * cardano_reward_address_t* reward_address = cardano_reward_address_new(...); // Assume reward_address is already initialized
 *
 * cardano_error_t result = cardano_proposal_procedure_set_reward_address(proposal_procedure, reward_address);
 * if (result == CARDANO_SUCCESS)
 * {
 *   // The reward_address is now set for the proposal_procedure
 * }
 * else
 * {
 *   printf("Failed to set the reward_address.\n");
 * }
 * // Clean up the proposal_procedure and reward_address after use
 * cardano_proposal_procedure_unref(&proposal_procedure);
 * cardano_reward_address_unref(&reward_address);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_proposal_procedure_set_reward_address(cardano_proposal_procedure_t* proposal_procedure, cardano_reward_address_t* reward_address);

/**
 * \brief Gets the reward address from a proposal_procedure.
 *
 * This function retrieves the reward address from a given \ref cardano_proposal_procedure_t object.
 *
 * \param[in] proposal_procedure A pointer to an initialized \ref cardano_proposal_procedure_t object from which the reward_address is retrieved.
 *
 * \return A pointer to the retrieved \ref cardano_reward_address_t object representing the reward address. This will be a new reference,
 *         and the caller is responsible for releasing it with \ref cardano_reward_address_unref when it is no longer needed.
 *         If the proposal_procedure does not have an reward_address set, NULL is returned.
 *
 * Usage Example:
 * \code{.c}
 * cardano_proposal_procedure_t* proposal_procedure = ...; // Assume initialized
 * cardano_reward_address_t* reward_address = cardano_proposal_procedure_get_reward_address(proposal_procedure);
 *
 * if (reward_address != NULL)
 * {
 *   // Use the reward_address
 *
 *   // Once done, ensure to clean up and release the reward_address
 *   cardano_reward_address_unref(&reward_address);
 * }
 * else
 * {
 *   printf("ERROR: No reward_address set for this proposal_procedure.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_reward_address_t*
cardano_proposal_procedure_get_reward_address(cardano_proposal_procedure_t* proposal_procedure);

/**
 * \brief Retrieves the deposit amount from an proposal procedure.
 *
 * This function retrieves the deposit amount that was associated with the proposal procedure.
 *
 * \param[in] certificate A constant pointer to an initialized \ref cardano_proposal_procedure_t object.
 *
 * \return The deposit amount in lovelaces. If the proposal procedure is NULL, the function will return 0, which should be
 *         handled appropriately by the caller to distinguish from a genuine deposit of 0 lovelaces.
 *
 * Usage Example:
 * \code{.c}
 * const cardano_proposal_procedure_t* proposal_procedure = ...; // Assume proposal_procedure is already initialized
 *
 * uint64_t deposit = cardano_proposal_procedure_get_deposit(proposal_procedure);
 * printf("Deposit for unregistration: %llu lovelaces\n", deposit);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT uint64_t cardano_proposal_procedure_get_deposit(const cardano_proposal_procedure_t* certificate);

/**
 * \brief Sets the deposit amount for an proposal procedure.
 *
 * This function sets the deposit amount required for unregistration of a stake credential.
 *
 * \param[in,out] certificate A pointer to an initialized \ref cardano_proposal_procedure_t object to which the deposit amount will be set.
 * \param[in] deposit The deposit amount in lovelaces to set for the unregistration.
 *
 * \return \ref cardano_error_t indicating the outcome of the operation. Returns \ref CARDANO_SUCCESS
 *         if the deposit amount was successfully set, or an appropriate error code indicating the failure reason,
 *         such as \ref CARDANO_ERROR_POINTER_IS_NULL if the input certificate pointer is NULL.
 *
 * Usage Example:
 * \code{.c}
 * cardano_proposal_procedure_t* proposal_procedure = ...; // Assume proposal_procedure is already initialized
 * uint64_t deposit = 2000000;
 *
 * cardano_error_t result = cardano_proposal_procedure_set_deposit(proposal_procedure, deposit);
 *
 * if (result == CARDANO_SUCCESS)
 * {
 *   printf("Deposit set successfully.\n");
 * }
 * else
 * {
 *   printf("Failed to set deposit.\n");
 * }
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT cardano_error_t
cardano_proposal_procedure_set_deposit(cardano_proposal_procedure_t* certificate, uint64_t deposit);

/**
 * \brief Decrements the reference count of a cardano_proposal_procedure_t object.
 *
 * This function is responsible for managing the lifecycle of a \ref cardano_proposal_procedure_t object
 * by decreasing its reference count. When the reference count reaches zero, the auth_committee_hot is
 * finalized; its associated resources are released, and its memory is deallocated.
 *
 * \param[in,out] auth_committee_hot A pointer to the pointer of the auth_committee_hot object. This double
 *                            indirection allows the function to set the caller's pointer to
 *                            NULL, avoiding dangling pointer issues after the object has been
 *                            freed.
 *
 * Usage Example:
 * \code{.c}
 * cardano_proposal_procedure_t* auth_committee_hot = cardano_proposal_procedure_new(major, minor);
 *
 * // Perform operations with the auth_committee_hot...
 *
 * cardano_proposal_procedure_unref(&auth_committee_hot);
 * // At this point, auth_committee_hot is NULL and cannot be used.
 * \endcode
 *
 * \note After calling \ref cardano_proposal_procedure_unref, the pointer to the \ref cardano_proposal_procedure_t object
 *       will be set to NULL to prevent its reuse.
 */
CARDANO_EXPORT void cardano_proposal_procedure_unref(cardano_proposal_procedure_t** auth_committee_hot);

/**
 * \brief Increases the reference count of the cardano_proposal_procedure_t object.
 *
 * This function is used to manually increment the reference count of an cardano_proposal_procedure_t
 * object, indicating that another part of the code has taken ownership of it. This
 * ensures the object remains allocated and valid until all owners have released their
 * reference by calling \ref cardano_proposal_procedure_unref.
 *
 * \param auth_committee_hot A pointer to the cardano_proposal_procedure_t object whose reference count is to be incremented.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming auth_committee_hot is a previously created auth_committee_hot object
 *
 * cardano_proposal_procedure_ref(auth_committee_hot);
 *
 * // Now auth_committee_hot can be safely used elsewhere without worrying about premature deallocation
 * \endcode
 *
 * \note Always ensure that for every call to \ref cardano_proposal_procedure_ref there is a corresponding
 * call to \ref cardano_proposal_procedure_unref to prevent memory leaks.
 */
CARDANO_EXPORT void cardano_proposal_procedure_ref(cardano_proposal_procedure_t* auth_committee_hot);

/**
 * \brief Retrieves the current reference count of the cardano_proposal_procedure_t object.
 *
 * This function returns the number of active references to an cardano_proposal_procedure_t object. It's useful
 * for debugging purposes or managing the lifecycle of the object in complex scenarios.
 *
 * \warning This function does not account for transitive references. A transitive reference
 * occurs when an object holds a reference to another object, rather than directly to the
 * cardano_proposal_procedure_t. As such, the reported count may not fully represent the total number
 * of conceptual references in cases where such transitive relationships exist.
 *
 * \param auth_committee_hot A pointer to the cardano_proposal_procedure_t object whose reference count is queried.
 *                    The object must not be NULL.
 *
 * \return The number of active references to the specified cardano_proposal_procedure_t object. If the object
 * is properly managed (i.e., every \ref cardano_proposal_procedure_ref call is matched with a
 * \ref cardano_proposal_procedure_unref call), this count should reach zero right before the object
 * is deallocated.
 *
 * Usage Example:
 * \code{.c}
 * // Assuming auth_committee_hot is a previously created auth_committee_hot object
 *
 * size_t ref_count = cardano_proposal_procedure_refcount(auth_committee_hot);
 *
 * printf("Reference count: %zu\n", ref_count);
 * \endcode
 */
CARDANO_NODISCARD
CARDANO_EXPORT size_t cardano_proposal_procedure_refcount(const cardano_proposal_procedure_t* auth_committee_hot);

/**
 * \brief Sets the last error message for a given cardano_proposal_procedure_t object.
 *
 * Records an error message in the auth_committee_hot's last_error buffer, overwriting any existing message.
 * This is useful for storing descriptive error information that can be later retrieved. The message
 * is truncated if it exceeds the buffer's capacity.
 *
 * \param[in] auth_committee_hot A pointer to the \ref cardano_proposal_procedure_t instance whose last error message is
 *                       to be set. If \c NULL, the function does nothing.
 * \param[in] message A null-terminated string containing the error message. If \c NULL, the auth_committee_hot's
 *                    last_error is set to an empty string, indicating no error.
 *
 * \note The error message is limited to 1023 characters, including the null terminator, due to the
 * fixed size of the last_error buffer.
 */
CARDANO_EXPORT void cardano_proposal_procedure_set_last_error(
  cardano_proposal_procedure_t* auth_committee_hot,
  const char*                   message);

/**
 * \brief Retrieves the last error message recorded for a specific auth_committee_hot.
 *
 * This function returns a pointer to the null-terminated string containing
 * the last error message set by \ref cardano_proposal_procedure_set_last_error for the given
 * auth_committee_hot. If no error message has been set, or if the last_error buffer was
 * explicitly cleared, an empty string is returned, indicating no error.
 *
 * \param[in] auth_committee_hot A pointer to the \ref cardano_proposal_procedure_t instance whose last error
 *                   message is to be retrieved. If the auth_committee_hot is NULL, the function
 *                   returns a generic error message indicating the null auth_committee_hot.
 *
 * \return A pointer to a null-terminated string containing the last error
 *         message for the specified auth_committee_hot. If the auth_committee_hot is NULL, "Object is NULL."
 *         is returned to indicate the error.
 *
 * \note The returned string points to internal storage within the object and
 *       must not be modified by the caller. The string remains valid until the
 *       next call to \ref cardano_proposal_procedure_set_last_error for the same auth_committee_hot, or until
 *       the auth_committee_hot is deallocated.
 */
CARDANO_NODISCARD
CARDANO_EXPORT const char* cardano_proposal_procedure_get_last_error(
  const cardano_proposal_procedure_t* auth_committee_hot);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // PROPOSAL_PROCEDURE_H