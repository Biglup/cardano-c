/**
 * \file drep_type.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_DREP_TYPE_H
#define BIGLUP_LABS_INCLUDE_CARDANO_DREP_TYPE_H

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief In order to participate in governance, a stake credential must be delegated to a DRep.
 * Ada holders will generally delegate their voting rights to a registered DRep that will
 * vote on their behalf.
 *
 * In addition, two pre-defined DRep options are available: `Abstain` and `No Confidence`.
 */
typedef enum
{
  /**
   * \brief A DRep identified by a stake key hash.
   */
  CARDANO_DREP_TYPE_KEY_HASH = 0,

  /**
   * \brief A DRep identified by a script hash.
   */
  CARDANO_DREP_TYPE_SCRIPT_HASH = 1,

  /**
   * \brief If an Ada holder delegates to Abstain, then their stake is actively marked as not
   * participating in governance.
   *
   * The effect of delegating to Abstain on chain is that the delegated stake will not be
   * considered to be a part of the active voting stake. However, the stake will be considered
   * to be registered for the purpose of the incentives that are described in
   * the following link:
   *
   * \par
   * [Incentives for Ada holders to delegate voting stake](https://github.com/cardano-foundation/CIPs/blob/master/CIP-1694/README.md#incentives-for-ada-holders-to-delegate-voting-stake).
   */
  CARDANO_DREP_TYPE_ABSTAIN = 2,

  /**
   * \brief If an Ada holder delegates to No Confidence, then their stake is counted as a Yes vote on
   * every No Confidence action and a No vote on every other action. The delegated stake will
   * be considered part of the active voting stake. It also serves as a directly auditable
   * measure of the confidence of Ada holders in the constitutional committee.
   */
  CARDANO_DREP_TYPE_NO_CONFIDENCE = 3,
} cardano_drep_type_t;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // BIGLUP_LABS_INCLUDE_CARDANO_DREP_TYPE_H