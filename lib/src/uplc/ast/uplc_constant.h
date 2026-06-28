/**
 * \file uplc_constant.h
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_UPLC_AST_UPLC_CONSTANT_H
#define BIGLUP_LABS_INCLUDE_CARDANO_UPLC_AST_UPLC_CONSTANT_H

/* INCLUDES ******************************************************************/

#include <cardano/buffer.h>
#include <cardano/common/bigint.h>
#include <cardano/error.h>
#include <cardano/export.h>
#include <cardano/plutus_data/plutus_data.h>
#include <cardano/typedefs.h>
#include "../arena/uplc_arena.h"
#include "../builtins/uplc_builtin.h"
#include "uplc_type.h"
#include "uplc_type_kind.h"

/* FORWARD DECLARATIONS ******************************************************/

/**
 * \brief A VM-internal, arena-allocated Plutus-data node.
 *
 * The data constant arm holds a pointer to this lean, arena-allocated node rather
 * than the heavyweight refcounted \ref cardano_plutus_data_t. The struct is defined
 * in the module-private header \c uplc_data.h; the public header keeps the arm
 * opaque, as the BLS arm keeps the blst point opaque.
 */
struct cardano_uplc_data_t;

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief A constant value tagged by its constant kind.
 *
 * The \c kind selects the active union arm. Integer constants reuse the library's
 * refcounted \ref cardano_bigint_t (for the rare value that does not fit inline).
 * Byte-string and string constants store an inline \c (data, size) descriptor that
 * points into arena memory: the bytes are bump-allocated from the arena that built
 * the constant and released with it, so the hot byte-string builtins neither malloc
 * nor refcount per result. The \c data pointer is NULL only for the empty
 * byte string or string, where \c size is 0 and the pointer is never dereferenced.
 * A data constant holds an opaque pointer to a VM-internal, arena-allocated
 * \c cardano_uplc_data_t node (defined in \c uplc_data.h): it is not refcounted and
 * is released with the arena, like the BLS arm.
 *
 * A BLS arm (\ref CARDANO_UPLC_TYPE_BLS_G1, \ref CARDANO_UPLC_TYPE_BLS_G2 or
 * \ref CARDANO_UPLC_TYPE_BLS_ML_RESULT) holds \c bls: an opaque, arena-allocated
 * blob carrying the in-memory blst point (\c blst_p1 for G1, \c blst_p2 for G2)
 * or pairing result (\c blst_fp12 for ML result). The blob is not refcounted and
 * is owned by the arena; the runtime in \c builtins.c casts it to the concrete
 * blst type. The public header stays free of the blst dependency by treating the
 * arm as opaque storage; see \ref cardano_uplc_constant_new_bls.
 */
typedef struct cardano_uplc_constant_t
{
  cardano_uplc_type_kind_t kind;

  union
  {
    /**
     * \brief An arbitrary-precision integer stored inline when it fits an \c int64_t.
     *
     * Most UPLC integers are tiny, so the machine keeps them as a flat \c int64_t
     * with no heap allocation; \c big is reserved for the rare value whose magnitude
     * exceeds the signed 64-bit range. When \c is_small is \c true the value is
     * exactly \c small and \c big is either NULL or a lazily materialized,
     * arena-owned copy of \c small built on demand by the builtins that need a
     * \ref cardano_bigint_t (a read-through cache, never the source of truth). When
     * \c is_small is \c false the value lives in \c big and \c small is unused. The
     * fast arithmetic path operates on \c small directly and falls back to \c big
     * only on overflow.
     */
    struct
    {
      int64_t           small;
      cardano_bigint_t* big;
      bool              is_small;
    } integer;

    struct
    {
      const byte_t* data;
      size_t        size;
    } bytes;

    struct
    {
      const byte_t* data;
      size_t        size;
    } string;

    bool                             boolean;
    const struct cardano_uplc_data_t* data;
    const void*                      bls;

    struct
    {
      const cardano_uplc_type_t*                   element_type;
      const struct cardano_uplc_constant_t* const* items;
      size_t                                       count;
    } list; /* also the active arm for CARDANO_UPLC_TYPE_ARRAY and CARDANO_UPLC_TYPE_VALUE */

    struct
    {
      const struct cardano_uplc_constant_t* fst;
      const struct cardano_uplc_constant_t* snd;
    } pair;
  } as;
} cardano_uplc_constant_t;

/**
 * \brief Builds an integer constant that borrows a refcounted bigint.
 *
 * Allocates the constant node from \p arena and registers \p value with the
 * arena so a single \ref cardano_uplc_arena_free releases the arena's reference.
 * The arena takes its own reference; the caller keeps the reference it passed in
 * and must still release it.
 *
 * \param[in] arena The arena to allocate the constant from.
 * \param[in] value The integer value. Must not be NULL.
 * \param[out] constant On success, set to the new constant; left untouched on failure.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         any argument is NULL, \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED if the
 *         arena cannot serve the node, or \ref CARDANO_ERROR_ILLEGAL_STATE if the
 *         arena is past its byte ceiling.
 */
cardano_error_t
cardano_uplc_constant_new_integer(
  struct cardano_uplc_arena_t* arena,
  cardano_bigint_t*            value,
  cardano_uplc_constant_t**    constant);

/**
 * \brief Builds an integer constant from an inline \c int64_t with no allocation.
 *
 * This is the fast path for the overwhelmingly common case of a small integer: the
 * value is stored inline in the constant node and no \ref cardano_bigint_t is
 * allocated. A \ref cardano_bigint_t is only ever built lazily, and cached in the
 * node, by the builtins that genuinely need one. Use this whenever the value is
 * known to fit \c int64_t; otherwise use \ref cardano_uplc_constant_new_integer.
 *
 * \param[in] arena The arena to allocate the constant from.
 * \param[in] value The inline integer value.
 * \param[out] constant On success, set to the new constant; left untouched on failure.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         \p arena or \p constant is NULL, or
 *         \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED if the arena cannot serve the
 *         node.
 */
cardano_error_t
cardano_uplc_constant_new_integer_small(
  struct cardano_uplc_arena_t* arena,
  int64_t                      value,
  cardano_uplc_constant_t**    constant);

/**
 * \brief Builds a byte-string constant by copying a buffer's bytes into the arena.
 *
 * Allocates the constant node and a copy of \p bytes from \p arena, both released by
 * a single \ref cardano_uplc_arena_free. The constant does not retain \p bytes;
 * nothing is refcounted. The copy is the boundary path the flat decoder and text
 * parser use to publish a freshly read buffer as an arena byte string.
 *
 * \param[in] arena The arena to allocate the constant and the byte copy from.
 * \param[in] bytes The byte-string contents. Must not be NULL.
 * \param[out] constant On success, set to the new constant; left untouched on failure.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         any argument is NULL, or \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED if the
 *         arena cannot serve the node or the byte copy.
 */
cardano_error_t
cardano_uplc_constant_new_byte_string(
  struct cardano_uplc_arena_t* arena,
  cardano_buffer_t*            bytes,
  cardano_uplc_constant_t**    constant);

/**
 * \brief Builds a UTF-8 string constant by copying a buffer's bytes into the arena.
 *
 * Allocates the constant node and a copy of \p string from \p arena, both released
 * by a single \ref cardano_uplc_arena_free; nothing is refcounted. The caller is
 * responsible for having validated the UTF-8; this constructor does not re-validate.
 *
 * \param[in] arena The arena to allocate the constant and the byte copy from.
 * \param[in] string The text contents. Must not be NULL.
 * \param[out] constant On success, set to the new constant; left untouched on failure.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         any argument is NULL, or \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED if the
 *         arena cannot serve the node or the byte copy.
 */
cardano_error_t
cardano_uplc_constant_new_string(
  struct cardano_uplc_arena_t* arena,
  cardano_buffer_t*            string,
  cardano_uplc_constant_t**    constant);

/**
 * \brief Builds a boolean constant.
 *
 * \param[in] arena The arena to allocate the constant from.
 * \param[in] value The boolean value.
 * \param[out] constant On success, set to the new constant; left untouched on failure.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         \p arena or \p constant is NULL, or
 *         \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED if the arena cannot serve
 *         the node.
 */
cardano_error_t
cardano_uplc_constant_new_bool(
  struct cardano_uplc_arena_t* arena,
  bool                         value,
  cardano_uplc_constant_t**    constant);

/**
 * \brief Builds the unit constant.
 *
 * \param[in] arena The arena to allocate the constant from.
 * \param[out] constant On success, set to the new constant; left untouched on failure.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         \p arena or \p constant is NULL, or
 *         \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED if the arena cannot serve
 *         the node.
 */
cardano_error_t
cardano_uplc_constant_new_unit(
  struct cardano_uplc_arena_t* arena,
  cardano_uplc_constant_t**    constant);

/**
 * \brief Builds a data constant that borrows a refcounted plutus-data object.
 *
 * Allocates the constant node from \p arena and registers \p data with the arena
 * so a single \ref cardano_uplc_arena_free releases the arena's reference. The
 * arena takes its own reference; the caller keeps the reference it passed in.
 *
 * \param[in] arena The arena to allocate the constant from.
 * \param[in] data The plutus-data value. Must not be NULL.
 * \param[out] constant On success, set to the new constant; left untouched on failure.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         any argument is NULL, \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED if the
 *         arena cannot serve the node, or \ref CARDANO_ERROR_ILLEGAL_STATE if the
 *         arena is past its byte ceiling.
 */
cardano_error_t
cardano_uplc_constant_new_data(
  struct cardano_uplc_arena_t* arena,
  cardano_plutus_data_t*       data,
  cardano_uplc_constant_t**    constant);

/**
 * \brief Builds a BLS12-381 constant from a raw in-memory blst value.
 *
 * Copies \p size bytes from \p data into the arena and stores the copy in the
 * \c bls arm. \p kind must be one of \ref CARDANO_UPLC_TYPE_BLS_G1,
 * \ref CARDANO_UPLC_TYPE_BLS_G2 or \ref CARDANO_UPLC_TYPE_BLS_ML_RESULT, and
 * \p data must point at the matching blst struct (\c blst_p1, \c blst_p2 or
 * \c blst_fp12). The copy is owned by the arena and released with it; it is not
 * refcounted.
 *
 * \param[in] arena The arena to allocate the constant and the blob from.
 * \param[in] kind The BLS constant kind.
 * \param[in] data The raw blst value to copy. Must not be NULL.
 * \param[in] size The number of bytes to copy from \p data.
 * \param[out] constant On success, set to the new constant; left untouched on failure.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         \p arena, \p data or \p constant is NULL, \ref CARDANO_ERROR_INVALID_ARGUMENT
 *         if \p kind is not a BLS kind or \p size is 0, or
 *         \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED if the arena cannot serve the
 *         node.
 */
cardano_error_t
cardano_uplc_constant_new_bls(
  struct cardano_uplc_arena_t* arena,
  cardano_uplc_type_kind_t     kind,
  const void*                  data,
  size_t                       size,
  cardano_uplc_constant_t**    constant);

/**
 * \brief Builds a typed list constant from an arena-allocated item array.
 *
 * The \p items array and the items it points at must already live in \p arena;
 * the constructor copies the pointers it is given by reference, it does not copy
 * the items. An empty list is allowed with \p count 0 and \p items NULL.
 *
 * \param[in] arena The arena to allocate the constant from.
 * \param[in] element_type The element type descriptor. Must not be NULL.
 * \param[in] items The element pointers, or NULL when \p count is 0.
 * \param[in] count The number of elements.
 * \param[out] constant On success, set to the new constant; left untouched on failure.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         \p arena, \p element_type, or \p constant is NULL,
 *         \ref CARDANO_ERROR_INVALID_ARGUMENT if \p items is NULL while
 *         \p count is non-zero, or \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED if
 *         the arena cannot serve the node.
 */
cardano_error_t
cardano_uplc_constant_new_list(
  struct cardano_uplc_arena_t*                 arena,
  const cardano_uplc_type_t*                   element_type,
  const cardano_uplc_constant_t* const*        items,
  size_t                                       count,
  cardano_uplc_constant_t**                    constant);

/**
 * \brief Builds a typed array constant from an arena-allocated item array.
 *
 * An array is stored exactly like a list (element type, item pointers and a
 * count) but carries the \ref CARDANO_UPLC_TYPE_ARRAY kind, which keeps it a
 * distinct constant form for the \c lengthOfArray, \c listToArray and
 * \c indexArray builtins and for the surface syntax. The \p items array and the
 * items it points at must already live in \p arena; the constructor copies the
 * pointers it is given by reference. An empty array is allowed with \p count 0
 * and \p items NULL.
 *
 * \param[in] arena The arena to allocate the constant from.
 * \param[in] element_type The element type descriptor. Must not be NULL.
 * \param[in] items The element pointers, or NULL when \p count is 0.
 * \param[in] count The number of elements.
 * \param[out] constant On success, set to the new constant; left untouched on failure.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         \p arena, \p element_type, or \p constant is NULL,
 *         \ref CARDANO_ERROR_INVALID_ARGUMENT if \p items is NULL while
 *         \p count is non-zero, or \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED if
 *         the arena cannot serve the node.
 */
cardano_error_t
cardano_uplc_constant_new_array(
  struct cardano_uplc_arena_t*                 arena,
  const cardano_uplc_type_t*                   element_type,
  const cardano_uplc_constant_t* const*        items,
  size_t                                       count,
  cardano_uplc_constant_t**                    constant);

/**
 * \brief Builds a multi-asset value constant from a normalized policy list.
 *
 * A value is stored exactly like a list (element type, item pointers and a count)
 * but carries the \ref CARDANO_UPLC_TYPE_VALUE kind. Each item must be a pair whose
 * first component is a byte-string policy and whose second component is a list of
 * pairs of (byte-string token, integer amount). The caller is responsible for the
 * invariants the V4 value builtins maintain (ascending keys, no duplicates, no zero
 * amounts, no empty inner maps); this constructor stores the items by reference. An
 * empty value is allowed with \p count 0 and \p items NULL. The \p element_type is
 * the canonical value element type and must not be NULL.
 *
 * \param[in] arena The arena to allocate the constant from.
 * \param[in] element_type The element type descriptor. Must not be NULL.
 * \param[in] items The policy-entry pointers, or NULL when \p count is 0.
 * \param[in] count The number of policy entries.
 * \param[out] constant On success, set to the new constant; left untouched on failure.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         \p arena, \p element_type, or \p constant is NULL,
 *         \ref CARDANO_ERROR_INVALID_ARGUMENT if \p items is NULL while \p count is
 *         non-zero, or \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED if the arena
 *         cannot serve the node.
 */
cardano_error_t
cardano_uplc_constant_new_value(
  struct cardano_uplc_arena_t*                 arena,
  const cardano_uplc_type_t*                   element_type,
  const cardano_uplc_constant_t* const*        items,
  size_t                                       count,
  cardano_uplc_constant_t**                    constant);

/**
 * \brief Builds a typed pair constant from two arena-allocated components.
 *
 * \param[in] arena The arena to allocate the constant from.
 * \param[in] fst The first component. Must not be NULL and must live in \p arena.
 * \param[in] snd The second component. Must not be NULL and must live in \p arena.
 * \param[out] constant On success, set to the new constant; left untouched on failure.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         any argument is NULL, or \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED if
 *         the arena cannot serve the node.
 */
cardano_error_t
cardano_uplc_constant_new_pair(
  struct cardano_uplc_arena_t*   arena,
  const cardano_uplc_constant_t* fst,
  const cardano_uplc_constant_t* snd,
  cardano_uplc_constant_t**      constant);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BIGLUP_LABS_INCLUDE_CARDANO_UPLC_AST_UPLC_CONSTANT_H */
