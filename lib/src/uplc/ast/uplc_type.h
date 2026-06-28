/**
 * \file uplc_type.h
 *
 * \author angel.castillo
 * \date   Jun 18, 2026
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_UPLC_AST_UPLC_TYPE_H
#define BIGLUP_LABS_INCLUDE_CARDANO_UPLC_AST_UPLC_TYPE_H

/* INCLUDES ******************************************************************/

#include <cardano/error.h>
#include <cardano/export.h>
#include <cardano/typedefs.h>
#include "../arena/uplc_arena.h"
#include "uplc_type_kind.h"

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief A constant type descriptor.
 *
 * Flat encodes the type of every constant, and list, array and pair constants
 * carry their element and component types, so the universe needs a recursive
 * type descriptor as well as a value. For a list or an array, \c fst is the
 * element type and \c snd is NULL; for a pair, \c fst and \c snd are the
 * component types; for every other kind both are NULL.
 */
typedef struct cardano_uplc_type_t
{
  cardano_uplc_type_kind_t   kind;
  const struct cardano_uplc_type_t* fst;
  const struct cardano_uplc_type_t* snd;
} cardano_uplc_type_t;

/**
 * \brief Builds a constant type descriptor from an arena.
 *
 * For \ref CARDANO_UPLC_TYPE_LIST and \ref CARDANO_UPLC_TYPE_ARRAY, \p fst is the
 * required element type and \p snd must be NULL. For \ref CARDANO_UPLC_TYPE_PAIR,
 * \p fst and \p snd are the required component types. For every other kind, both
 * \p fst and \p snd must be NULL.
 *
 * \param[in] arena The arena to allocate the descriptor from.
 * \param[in] kind The constant kind the descriptor names.
 * \param[in] fst The element type for a list, the first type for a pair, else NULL.
 * \param[in] snd The second type for a pair, else NULL.
 * \param[out] type On success, set to the new descriptor; left untouched on failure.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         \p arena, \p type, or a required component type is NULL,
 *         \ref CARDANO_ERROR_INVALID_ARGUMENT if a component type is supplied
 *         where the kind forbids it, or \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED
 *         if the arena cannot serve the node.
 */
cardano_error_t
cardano_uplc_type_new(
  struct cardano_uplc_arena_t* arena,
  cardano_uplc_type_kind_t     kind,
  const cardano_uplc_type_t*   fst,
  const cardano_uplc_type_t*   snd,
  cardano_uplc_type_t**        type);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BIGLUP_LABS_INCLUDE_CARDANO_UPLC_AST_UPLC_TYPE_H */
