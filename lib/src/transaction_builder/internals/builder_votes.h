/**
 * \file builder_votes.h
 *
 * \author angel.castillo
 * \date   Jul 18, 2026
 *
 * Copyright 2026 Biglup Labs
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_BUILDER_VOTES_H
#define BIGLUP_LABS_INCLUDE_CARDANO_BUILDER_VOTES_H

/* INCLUDES ******************************************************************/

#include <cardano/common/governance_action_id.h>
#include <cardano/error.h>
#include <cardano/plutus_data/plutus_data.h>
#include <cardano/transaction_builder/balancing/deferred_redeemer_list.h>
#include <cardano/voting_procedures/voter.h>
#include <cardano/voting_procedures/voting_procedure.h>

#include "builder_state.h"

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Records a vote for a governance action in the transaction.
 *
 * This function inserts the voting procedure for \p voter and \p action_id into the voting
 * procedures of the transaction body, creating the collection when it is missing. When
 * \p redeemer is not NULL it is attached as a voting redeemer keyed by the sortable id of the
 * voter in the state vote redeemer map. When \p redeemer is NULL the sortable id is still
 * recorded in the map with a NULL redeemer so redeemer indices can be computed.
 *
 * \param[in,out] state A pointer to the \ref cardano_builder_state_t tracking the transaction under
 *                      construction. This parameter must not be NULL.
 * \param[in] voter A pointer to the \ref cardano_voter_t casting the vote. This parameter must not
 *                  be NULL.
 * \param[in] action_id A pointer to the \ref cardano_governance_action_id_t of the governance action
 *                      being voted on. This parameter must not be NULL.
 * \param[in] vote A pointer to the \ref cardano_voting_procedure_t with the vote to record. This
 *                 parameter must not be NULL.
 * \param[in] redeemer A pointer to the \ref cardano_plutus_data_t used as voting redeemer payload,
 *                     or NULL when the voter does not require one.
 * \param[out] error_message A pointer that receives a static string describing the failure when the
 *                           function does not return \ref CARDANO_SUCCESS. It is left untouched on
 *                           success. This parameter must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the vote was recorded, or an appropriate error code indicating
 *         the failure reason.
 */
cardano_error_t
cardano_builder_vote(
  cardano_builder_state_t*        state,
  cardano_voter_t*                voter,
  cardano_governance_action_id_t* action_id,
  cardano_voting_procedure_t*     vote,
  cardano_plutus_data_t*          redeemer,
  const char**                    error_message);

/**
 * \brief Records a vote whose redeemer payload is resolved during balancing.
 *
 * This function records the vote with an empty placeholder redeemer and registers the callback in
 * the state deferred redeemer list keyed by the sortable id of the voter. The callback is invoked
 * while the transaction is balanced to produce the final redeemer payload.
 *
 * \param[in,out] state A pointer to the \ref cardano_builder_state_t tracking the transaction under
 *                      construction. This parameter must not be NULL.
 * \param[in] voter A pointer to the \ref cardano_voter_t casting the vote. This parameter must not
 *                  be NULL.
 * \param[in] action_id A pointer to the \ref cardano_governance_action_id_t of the governance action
 *                      being voted on. This parameter must not be NULL.
 * \param[in] vote A pointer to the \ref cardano_voting_procedure_t with the vote to record. This
 *                 parameter must not be NULL.
 * \param[in] callback The deferred redeemer callback invoked during balancing. This parameter must
 *                     not be NULL.
 * \param[in] user_context The opaque pointer forwarded to the callback.
 * \param[out] error_message A pointer that receives a static string describing the failure when the
 *                           function does not return \ref CARDANO_SUCCESS. It is left untouched on
 *                           success. This parameter must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS if the vote and callback were registered, or an appropriate error
 *         code indicating the failure reason.
 */
cardano_error_t
cardano_builder_vote_with_deferred_redeemer(
  cardano_builder_state_t*        state,
  cardano_voter_t*                voter,
  cardano_governance_action_id_t* action_id,
  cardano_voting_procedure_t*     vote,
  cardano_deferred_redeemer_fn_t  callback,
  void*                           user_context,
  const char**                    error_message);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_BUILDER_VOTES_H
