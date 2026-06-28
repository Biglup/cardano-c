/**
 * \file uplc_frame.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_UPLC_MACHINE_UPLC_FRAME_H
#define BIGLUP_LABS_INCLUDE_CARDANO_UPLC_MACHINE_UPLC_FRAME_H

/* INCLUDES ******************************************************************/

#include "uplc_env.h"
#include "uplc_frame_kind.h"
#include "uplc_value.h"

#include <cardano/error.h>
#include <cardano/typedefs.h>
#include "../arena/uplc_arena.h"
#include "../ast/uplc_term.h"

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct cardano_uplc_frame_t cardano_uplc_frame_t;

/**
 * \brief An immutable CEK continuation frame, tagged by its frame kind.
 *
 * Frames form the continuation chain: every non-empty frame holds a \c ctx
 * pointing at the enclosing frame, terminating at a \ref CARDANO_UPLC_FRAME_NO_FRAME
 * frame whose \c ctx is NULL. The \c kind selects the active arm; never read an
 * arm the kind does not select. Frames are arena-allocated and immutable.
 */
struct cardano_uplc_frame_t
{
  cardano_uplc_frame_kind_t kind;

  union
  {
    struct
    {
      const cardano_uplc_value_t* value;
      const cardano_uplc_frame_t* ctx;
    } await_arg;

    struct
    {
      const cardano_uplc_env_t*   env;
      const cardano_uplc_term_t*  term;
      const cardano_uplc_frame_t* ctx;
    } await_fun_term;

    struct
    {
      const cardano_uplc_value_t* value;
      const cardano_uplc_frame_t* ctx;
    } await_fun_value;

    struct
    {
      const cardano_uplc_frame_t* ctx;
    } force;

    struct
    {
      const cardano_uplc_env_t*          env;
      uint64_t                           tag;
      const cardano_uplc_term_t* const*  fields;
      size_t                             field_count;
      const cardano_uplc_value_t* const* resolved;
      size_t                             resolved_count;
      const cardano_uplc_frame_t*        ctx;
    } constr;

    struct
    {
      const cardano_uplc_env_t*         env;
      const cardano_uplc_term_t* const* branches;
      size_t                            branch_count;
      const cardano_uplc_frame_t*       ctx;
    } cases;
  } as;
};

/**
 * \brief Builds the empty continuation frame.
 *
 * Returning a value into this frame ends evaluation. Its \c ctx is NULL.
 *
 * \param[in] arena The arena to allocate the frame from. Must not be NULL.
 * \param[out] out On success, set to the new frame; left untouched on failure.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         \p arena or \p out is NULL, or
 *         \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED if the arena cannot serve
 *         the node.
 */
cardano_error_t
cardano_uplc_frame_new_no_frame(
  struct cardano_uplc_arena_t* arena,
  cardano_uplc_frame_t**       out);

/**
 * \brief Builds an await-argument frame holding a function value.
 *
 * \param[in] arena The arena to allocate the frame from. Must not be NULL.
 * \param[in] value The function value to apply once the argument is computed.
 *            Must not be NULL and must live in \p arena.
 * \param[in] ctx The enclosing frame. Must not be NULL and must live in \p arena.
 * \param[out] out On success, set to the new frame; left untouched on failure.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         any argument is NULL, or \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED if
 *         the arena cannot serve the node.
 */
cardano_error_t
cardano_uplc_frame_new_await_arg(
  struct cardano_uplc_arena_t* arena,
  const cardano_uplc_value_t*  value,
  const cardano_uplc_frame_t*  ctx,
  cardano_uplc_frame_t**       out);

/**
 * \brief Builds an await-function-term frame holding the argument term and env.
 *
 * \param[in] arena The arena to allocate the frame from. Must not be NULL.
 * \param[in] env The environment the argument term is computed in, or NULL for
 *            the empty environment.
 * \param[in] term The argument term still to be computed. Must not be NULL and
 *            must live in \p arena.
 * \param[in] ctx The enclosing frame. Must not be NULL and must live in \p arena.
 * \param[out] out On success, set to the new frame; left untouched on failure.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         \p arena, \p term, \p ctx, or \p out is NULL, or
 *         \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED if the arena cannot serve
 *         the node.
 */
cardano_error_t
cardano_uplc_frame_new_await_fun_term(
  struct cardano_uplc_arena_t* arena,
  const cardano_uplc_env_t*    env,
  const cardano_uplc_term_t*   term,
  const cardano_uplc_frame_t*  ctx,
  cardano_uplc_frame_t**       out);

/**
 * \brief Builds an await-function-value frame holding an argument value.
 *
 * \param[in] arena The arena to allocate the frame from. Must not be NULL.
 * \param[in] value The argument value held while the function is computed. Must
 *            not be NULL and must live in \p arena.
 * \param[in] ctx The enclosing frame. Must not be NULL and must live in \p arena.
 * \param[out] out On success, set to the new frame; left untouched on failure.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         any argument is NULL, or \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED if
 *         the arena cannot serve the node.
 */
cardano_error_t
cardano_uplc_frame_new_await_fun_value(
  struct cardano_uplc_arena_t* arena,
  const cardano_uplc_value_t*  value,
  const cardano_uplc_frame_t*  ctx,
  cardano_uplc_frame_t**       out);

/**
 * \brief Builds a force frame.
 *
 * \param[in] arena The arena to allocate the frame from. Must not be NULL.
 * \param[in] ctx The enclosing frame. Must not be NULL and must live in \p arena.
 * \param[out] out On success, set to the new frame; left untouched on failure.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         \p arena, \p ctx, or \p out is NULL, or
 *         \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED if the arena cannot serve
 *         the node.
 */
cardano_error_t
cardano_uplc_frame_new_force(
  struct cardano_uplc_arena_t* arena,
  const cardano_uplc_frame_t*  ctx,
  cardano_uplc_frame_t**       out);

/**
 * \brief Builds a constructor frame for evaluating fields one at a time.
 *
 * \p fields is the list of field terms still to be computed and \p resolved is
 * the list of field values computed so far; both arrays and the nodes they point
 * at must live in \p arena. Either may be empty (count 0 with a NULL array).
 *
 * \param[in] arena The arena to allocate the frame from. Must not be NULL.
 * \param[in] env The environment the remaining field terms are computed in, or
 *            NULL for the empty environment.
 * \param[in] tag The constructor tag being built.
 * \param[in] fields The field terms still to compute, or NULL when
 *            \p field_count is 0.
 * \param[in] field_count The number of remaining field terms.
 * \param[in] resolved The field values resolved so far, or NULL when
 *            \p resolved_count is 0.
 * \param[in] resolved_count The number of resolved field values.
 * \param[in] ctx The enclosing frame. Must not be NULL and must live in \p arena.
 * \param[out] out On success, set to the new frame; left untouched on failure.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         \p arena, \p ctx, or \p out is NULL, \ref CARDANO_ERROR_INVALID_ARGUMENT
 *         if a count is non-zero while its array is NULL, or
 *         \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED if the arena cannot serve
 *         the node.
 */
cardano_error_t
cardano_uplc_frame_new_constr(
  struct cardano_uplc_arena_t*       arena,
  const cardano_uplc_env_t*          env,
  uint64_t                           tag,
  const cardano_uplc_term_t* const*  fields,
  size_t                             field_count,
  const cardano_uplc_value_t* const* resolved,
  size_t                             resolved_count,
  const cardano_uplc_frame_t*        ctx,
  cardano_uplc_frame_t**             out);

/**
 * \brief Builds a cases frame holding the case branches.
 *
 * The \p branches array and the terms it points at must live in \p arena. A case
 * with no branches is allowed with \p branch_count 0 and \p branches NULL.
 *
 * \param[in] arena The arena to allocate the frame from. Must not be NULL.
 * \param[in] env The environment the chosen branch is computed in, or NULL for
 *            the empty environment.
 * \param[in] branches The branch terms, or NULL when \p branch_count is 0.
 * \param[in] branch_count The number of branches.
 * \param[in] ctx The enclosing frame. Must not be NULL and must live in \p arena.
 * \param[out] out On success, set to the new frame; left untouched on failure.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         \p arena, \p ctx, or \p out is NULL, \ref CARDANO_ERROR_INVALID_ARGUMENT
 *         if \p branches is NULL while \p branch_count is non-zero, or
 *         \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED if the arena cannot serve
 *         the node.
 */
cardano_error_t
cardano_uplc_frame_new_cases(
  struct cardano_uplc_arena_t*      arena,
  const cardano_uplc_env_t*         env,
  const cardano_uplc_term_t* const* branches,
  size_t                            branch_count,
  const cardano_uplc_frame_t*       ctx,
  cardano_uplc_frame_t**            out);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BIGLUP_LABS_INCLUDE_CARDANO_UPLC_MACHINE_UPLC_FRAME_H */
