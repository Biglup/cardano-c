/**
 * \file uplc_term.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_UPLC_AST_UPLC_TERM_H
#define BIGLUP_LABS_INCLUDE_CARDANO_UPLC_AST_UPLC_TERM_H

/* INCLUDES ******************************************************************/

#include <cardano/error.h>
#include <cardano/export.h>
#include <cardano/typedefs.h>
#include "../arena/uplc_arena.h"
#include "../builtins/uplc_builtin.h"
#include "uplc_constant.h"
#include "uplc_term_kind.h"

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief An immutable Untyped Plutus Core term, tagged by its term kind.
 *
 * Variables are de Bruijn indices, so terms carry no binder names: a lambda
 * stores only its body and introduces one de Bruijn level. The \c kind selects
 * the active union arm; never read an arm the kind does not select. Terms are
 * allocated from an arena and are immutable after construction.
 */
typedef struct cardano_uplc_term_t
{
  cardano_uplc_term_kind_t kind;

  union
  {
    uint64_t                       var_index;
    const struct cardano_uplc_term_t* unary;
    const cardano_uplc_constant_t* constant;
    cardano_uplc_builtin_t         builtin;

    struct
    {
      const struct cardano_uplc_term_t* function;
      const struct cardano_uplc_term_t* argument;
    } apply;

    struct
    {
      uint64_t                                 tag;
      const struct cardano_uplc_term_t* const* fields;
      size_t                                   field_count;
    } constr;

    struct
    {
      const struct cardano_uplc_term_t*        scrutinee;
      const struct cardano_uplc_term_t* const* branches;
      size_t                                   branch_count;
    } cases;
  } as;
} cardano_uplc_term_t;

/**
 * \brief Builds a variable term from a de Bruijn index.
 *
 * \param[in] arena The arena to allocate the term from.
 * \param[in] index The de Bruijn index; 1 names the innermost binder.
 * \param[out] term On success, set to the new term; left untouched on failure.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         \p arena or \p term is NULL, or
 *         \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED if the arena cannot serve
 *         the node.
 */
cardano_error_t
cardano_uplc_term_new_var(
  struct cardano_uplc_arena_t* arena,
  uint64_t                     index,
  cardano_uplc_term_t**        term);

/**
 * \brief Builds a delay term wrapping a body term.
 *
 * \param[in] arena The arena to allocate the term from.
 * \param[in] body The delayed body. Must not be NULL and must live in \p arena.
 * \param[out] term On success, set to the new term; left untouched on failure.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         any argument is NULL, or \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED if
 *         the arena cannot serve the node.
 */
cardano_error_t
cardano_uplc_term_new_delay(
  struct cardano_uplc_arena_t* arena,
  const cardano_uplc_term_t*   body,
  cardano_uplc_term_t**        term);

/**
 * \brief Builds a lambda term wrapping a body term.
 *
 * The binder is implicit: the lambda introduces one de Bruijn level and stores
 * only \p body.
 *
 * \param[in] arena The arena to allocate the term from.
 * \param[in] body The lambda body. Must not be NULL and must live in \p arena.
 * \param[out] term On success, set to the new term; left untouched on failure.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         any argument is NULL, or \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED if
 *         the arena cannot serve the node.
 */
cardano_error_t
cardano_uplc_term_new_lambda(
  struct cardano_uplc_arena_t* arena,
  const cardano_uplc_term_t*   body,
  cardano_uplc_term_t**        term);

/**
 * \brief Builds an application term from a function and an argument.
 *
 * \param[in] arena The arena to allocate the term from.
 * \param[in] function The function term. Must not be NULL and must live in \p arena.
 * \param[in] argument The argument term. Must not be NULL and must live in \p arena.
 * \param[out] term On success, set to the new term; left untouched on failure.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         any argument is NULL, or \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED if
 *         the arena cannot serve the node.
 */
cardano_error_t
cardano_uplc_term_new_apply(
  struct cardano_uplc_arena_t* arena,
  const cardano_uplc_term_t*   function,
  const cardano_uplc_term_t*   argument,
  cardano_uplc_term_t**        term);

/**
 * \brief Builds a constant term wrapping a constant.
 *
 * \param[in] arena The arena to allocate the term from.
 * \param[in] constant The constant. Must not be NULL and must live in \p arena.
 * \param[out] term On success, set to the new term; left untouched on failure.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         any argument is NULL, or \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED if
 *         the arena cannot serve the node.
 */
cardano_error_t
cardano_uplc_term_new_constant(
  struct cardano_uplc_arena_t*   arena,
  const cardano_uplc_constant_t* constant,
  cardano_uplc_term_t**          term);

/**
 * \brief Builds a force term wrapping a body term.
 *
 * \param[in] arena The arena to allocate the term from.
 * \param[in] body The forced body. Must not be NULL and must live in \p arena.
 * \param[out] term On success, set to the new term; left untouched on failure.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         any argument is NULL, or \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED if
 *         the arena cannot serve the node.
 */
cardano_error_t
cardano_uplc_term_new_force(
  struct cardano_uplc_arena_t* arena,
  const cardano_uplc_term_t*   body,
  cardano_uplc_term_t**        term);

/**
 * \brief Builds the error term.
 *
 * \param[in] arena The arena to allocate the term from.
 * \param[out] term On success, set to the new term; left untouched on failure.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         \p arena or \p term is NULL, or
 *         \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED if the arena cannot serve
 *         the node.
 */
cardano_error_t
cardano_uplc_term_new_error(
  struct cardano_uplc_arena_t* arena,
  cardano_uplc_term_t**        term);

/**
 * \brief Builds a builtin term naming a default function.
 *
 * \param[in] arena The arena to allocate the term from.
 * \param[in] builtin The builtin tag.
 * \param[out] term On success, set to the new term; left untouched on failure.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         \p arena or \p term is NULL, or
 *         \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED if the arena cannot serve
 *         the node.
 */
cardano_error_t
cardano_uplc_term_new_builtin(
  struct cardano_uplc_arena_t* arena,
  cardano_uplc_builtin_t       builtin,
  cardano_uplc_term_t**        term);

/**
 * \brief Builds a constructor term from a tag and a field array.
 *
 * The \p fields array and the terms it points at must already live in \p arena.
 * An empty constructor is allowed with \p field_count 0 and \p fields NULL.
 *
 * \param[in] arena The arena to allocate the term from.
 * \param[in] tag The constructor tag.
 * \param[in] fields The field pointers, or NULL when \p field_count is 0.
 * \param[in] field_count The number of fields.
 * \param[out] term On success, set to the new term; left untouched on failure.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         \p arena or \p term is NULL, \ref CARDANO_ERROR_INVALID_ARGUMENT if
 *         \p fields is NULL while \p field_count is non-zero, or
 *         \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED if the arena cannot serve
 *         the node.
 */
cardano_error_t
cardano_uplc_term_new_constr(
  struct cardano_uplc_arena_t*             arena,
  uint64_t                                 tag,
  const cardano_uplc_term_t* const*        fields,
  size_t                                   field_count,
  cardano_uplc_term_t**                    term);

/**
 * \brief Builds a case term from a scrutinee and a branch array.
 *
 * The \p branches array and the terms it points at must already live in \p arena.
 * A case with no branches is allowed with \p branch_count 0 and \p branches NULL.
 *
 * \param[in] arena The arena to allocate the term from.
 * \param[in] scrutinee The scrutinee term. Must not be NULL and must live in \p arena.
 * \param[in] branches The branch pointers, or NULL when \p branch_count is 0.
 * \param[in] branch_count The number of branches.
 * \param[out] term On success, set to the new term; left untouched on failure.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         \p arena, \p scrutinee, or \p term is NULL,
 *         \ref CARDANO_ERROR_INVALID_ARGUMENT if \p branches is NULL while
 *         \p branch_count is non-zero, or \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED
 *         if the arena cannot serve the node.
 */
cardano_error_t
cardano_uplc_term_new_case(
  struct cardano_uplc_arena_t*             arena,
  const cardano_uplc_term_t*               scrutinee,
  const cardano_uplc_term_t* const*        branches,
  size_t                                   branch_count,
  cardano_uplc_term_t**                    term);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BIGLUP_LABS_INCLUDE_CARDANO_UPLC_AST_UPLC_TERM_H */
