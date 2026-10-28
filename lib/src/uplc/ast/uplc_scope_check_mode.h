/**
 * \file uplc_scope_check_mode.h
 *
 * \author angel.castillo
 * \date   Oct 28, 2026
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_UPLC_AST_UPLC_SCOPE_CHECK_MODE_H
#define BIGLUP_LABS_INCLUDE_CARDANO_UPLC_AST_UPLC_SCOPE_CHECK_MODE_H

/* INCLUDES ******************************************************************/

#include <cardano/typedefs.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief Which subterms the scope check of a term visits.
 *
 * The ledger checks that a script is closed before evaluating it. Before the
 * Dijkstra protocol version that check did not look inside \c constr and \c case
 * terms, so a free variable there was only caught if the machine reached it; from
 * Dijkstra on every subterm is checked.
 */
typedef enum
{
  /** \brief Skips the fields of \c constr and the scrutinee and branches of \c case. */
  CARDANO_UPLC_SCOPE_CHECK_NO_CASE_CONSTR = 0,
  /** \brief Checks every subterm. */
  CARDANO_UPLC_SCOPE_CHECK_FULL = 1
} cardano_uplc_scope_check_mode_t;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BIGLUP_LABS_INCLUDE_CARDANO_UPLC_AST_UPLC_SCOPE_CHECK_MODE_H */
