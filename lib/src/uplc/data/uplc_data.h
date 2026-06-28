/**
 * \file uplc_data.h
 *
 * \author angel.castillo
 * \date   Jun 19, 2026
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

#ifndef BIGLUP_LABS_INCLUDE_CARDANO_UPLC_DATA_UPLC_DATA_H
#define BIGLUP_LABS_INCLUDE_CARDANO_UPLC_DATA_UPLC_DATA_H

/* INCLUDES ******************************************************************/

#include <cardano/buffer.h>
#include <cardano/cbor/cbor_reader.h>
#include <cardano/cbor/cbor_writer.h>
#include <cardano/common/bigint.h>
#include <cardano/error.h>
#include <cardano/plutus_data/plutus_data.h>
#include <cardano/typedefs.h>

#include "../arena/uplc_arena.h"

#include "uplc_data_kind.h"
#include "uplc_data_pair.h"

#include <stdbool.h>
#include <stdint.h>

/* DECLARATIONS **************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * \brief A lean, arena-allocated Plutus-data node walked directly by the CEK
 *        machine.
 *
 * Unlike the library's refcounted \ref cardano_plutus_data_t, this node carries
 * no CBOR cache and is never refcounted: every node and every interior array is
 * served from the interpreter arena and released in one
 * \ref cardano_uplc_arena_free. The \c kind selects the active union arm.
 *
 * Integers reuse the inline-small representation from the integer constants: a
 * value that fits an \c int64_t lives in \c integer.small with \c integer.is_small
 * set and \c integer.big NULL; a larger value lives in \c integer.big. A bigint is
 * only ever built lazily, and cached in \c integer.big, by code that genuinely
 * needs a \ref cardano_bigint_t.
 *
 * The \c ex_mem field memoizes the data ex-mem of the subtree rooted at this node:
 * it is \c -1 until the first \ref cardano_uplc_data_ex_mem call fills it, after
 * which the value is reused with no re-walk. The \c node_count field memoizes the
 * subtree node count the same way.
 */
typedef struct cardano_uplc_data_t
{
    cardano_uplc_data_kind_t kind;
    int64_t                  ex_mem;
    int64_t                  node_count;

    // cppcheck-suppress misra-c2012-19.2; Reason: tagged union is the VM value and cost-shape representation
    union
    {
        struct
        {
            uint64_t                                 tag;
            const struct cardano_uplc_data_t* const* fields;
            size_t                                   count;
        } constr;

        struct
        {
            const cardano_uplc_data_pair_t* entries;
            size_t                          count;
            bool                            indefinite;
        } map;

        struct
        {
            const struct cardano_uplc_data_t* const* items;
            size_t                                   count;
        } list;

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

        // cppcheck-suppress misra-c2012-19.2; Reason: tagged union is the VM value and cost-shape representation
    } as;
} cardano_uplc_data_t;

/**
 * \brief Builds an arena constructor data node from an arena-allocated field array.
 *
 * \param[in] arena The arena to allocate the node from.
 * \param[in] tag The constructor alternative tag.
 * \param[in] fields The field pointers, or NULL when \p count is 0. The array and
 *            the nodes it points at must already live in \p arena.
 * \param[in] count The number of fields.
 * \param[out] out On success, set to the new node; left untouched on failure.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         \p arena or \p out is NULL, \ref CARDANO_ERROR_INVALID_ARGUMENT if
 *         \p fields is NULL while \p count is non-zero, or
 *         \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED if the arena cannot serve the
 *         node.
 */
cardano_error_t
cardano_uplc_data_new_constr(
  cardano_uplc_arena_t*             arena,
  uint64_t                          tag,
  const cardano_uplc_data_t* const* fields,
  size_t                            count,
  cardano_uplc_data_t**             out);

/**
 * \brief Builds an arena map data node from an arena-allocated entry array.
 *
 * \param[in] arena The arena to allocate the node from.
 * \param[in] entries The (key, value) entries, or NULL when \p count is 0. The
 *            array and the nodes it points at must already live in \p arena.
 * \param[in] count The number of entries.
 * \param[in] indefinite Whether the map round-trips as a CBOR indefinite-length map.
 * \param[out] out On success, set to the new node; left untouched on failure.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         \p arena or \p out is NULL, \ref CARDANO_ERROR_INVALID_ARGUMENT if
 *         \p entries is NULL while \p count is non-zero, or
 *         \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED if the arena cannot serve the
 *         node.
 */
cardano_error_t
cardano_uplc_data_new_map(
  cardano_uplc_arena_t*           arena,
  const cardano_uplc_data_pair_t* entries,
  size_t                          count,
  bool                            indefinite,
  cardano_uplc_data_t**           out);

/**
 * \brief Builds an arena list data node from an arena-allocated item array.
 *
 * \param[in] arena The arena to allocate the node from.
 * \param[in] items The item pointers, or NULL when \p count is 0. The array and
 *            the nodes it points at must already live in \p arena.
 * \param[in] count The number of items.
 * \param[out] out On success, set to the new node; left untouched on failure.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         \p arena or \p out is NULL, \ref CARDANO_ERROR_INVALID_ARGUMENT if
 *         \p items is NULL while \p count is non-zero, or
 *         \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED if the arena cannot serve the
 *         node.
 */
cardano_error_t
cardano_uplc_data_new_list(
  cardano_uplc_arena_t*             arena,
  const cardano_uplc_data_t* const* items,
  size_t                            count,
  cardano_uplc_data_t**             out);

/**
 * \brief Builds an arena integer data node from an inline \c int64_t value.
 *
 * \param[in] arena The arena to allocate the node from.
 * \param[in] value The inline integer value.
 * \param[out] out On success, set to the new node; left untouched on failure.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         \p arena or \p out is NULL, or \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED
 *         if the arena cannot serve the node.
 */
cardano_error_t
cardano_uplc_data_new_integer_small(
  cardano_uplc_arena_t* arena,
  int64_t               value,
  cardano_uplc_data_t** out);

/**
 * \brief Builds an arena integer data node from a refcounted bigint.
 *
 * The node stores \p value inline as an \c int64_t when it fits; otherwise it
 * registers \p value with \p arena (which takes its own reference) and stores it in
 * \c integer.big. The caller keeps the reference it passed in.
 *
 * \param[in] arena The arena to allocate the node from.
 * \param[in] value The integer value. Must not be NULL.
 * \param[out] out On success, set to the new node; left untouched on failure.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         any argument is NULL, or \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED if
 *         the arena cannot serve the node or register the bigint.
 */
cardano_error_t
cardano_uplc_data_new_integer(
  cardano_uplc_arena_t* arena,
  cardano_bigint_t*     value,
  cardano_uplc_data_t** out);

/**
 * \brief Builds an arena byte-string data node, copying the bytes into the arena.
 *
 * \param[in] arena The arena to allocate the node and the byte copy from.
 * \param[in] bytes The byte-string contents, or NULL when \p size is 0.
 * \param[in] size The number of bytes.
 * \param[out] out On success, set to the new node; left untouched on failure.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         \p arena or \p out is NULL, \ref CARDANO_ERROR_INVALID_ARGUMENT if
 *         \p bytes is NULL while \p size is non-zero, or
 *         \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED if the arena cannot serve the
 *         node.
 */
cardano_error_t
cardano_uplc_data_new_bytes(
  cardano_uplc_arena_t* arena,
  const byte_t*         bytes,
  size_t                size,
  cardano_uplc_data_t** out);

/**
 * \brief Returns the bigint view of an integer data node, building one lazily.
 *
 * For a node whose value already lives in \c integer.big this returns it directly.
 * For an inline node it builds a bigint equal to \c integer.small, registers it with
 * \p arena (which takes ownership), caches it in the node, and returns it; later
 * calls return the cached pointer. The returned bigint is owned by the arena and the
 * caller must not unref it.
 *
 * \param[in] arena The arena the lazily built bigint is registered with.
 * \param[in] data An integer data node. Must not be NULL and must be of kind
 *            \ref CARDANO_UPLC_DATA_KIND_INTEGER.
 * \param[out] out On success, the bigint view; untouched on failure.
 *
 * \return \ref CARDANO_SUCCESS on success, or
 *         \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED if a bigint cannot be built or
 *         registered.
 */
cardano_error_t
cardano_uplc_data_integer_materialize(
  cardano_uplc_arena_t*      arena,
  const cardano_uplc_data_t* data,
  const cardano_bigint_t**   out);

/**
 * \brief Tests two arena data nodes for structural equality.
 *
 * Compares kind, then recursively compares structure with no CBOR round-trip:
 * constructor tag and ordered fields, map entries in order, list items in order,
 * integer value (inline or bigint), and byte content. Mirrors the structural
 * equality of \ref cardano_plutus_data_equals on the same tree.
 *
 * \param[in] lhs The first node, or NULL.
 * \param[in] rhs The second node, or NULL.
 *
 * \return \c true when the two trees are structurally equal, \c false otherwise.
 *         Two NULL pointers compare equal; a NULL and a non-NULL compare unequal.
 */
bool
cardano_uplc_data_equals(const cardano_uplc_data_t* lhs, const cardano_uplc_data_t* rhs);

/**
 * \brief Returns the memoized data ex-mem of a node, computing it on first call.
 *
 * Walks the tree once charging the per-node and per-leaf cost identical to
 * \ref cardano_uplc_data_ex_mem on the library type, caching the result on the node
 * so later calls return it with no re-walk. A NULL node measures 0.
 *
 * \param[in] data The data node to measure, or NULL.
 *
 * \return The data node's ex-mem size.
 */
int64_t
cardano_uplc_data_node_ex_mem(const cardano_uplc_data_t* data);

/**
 * \brief Returns the memoized total node count of a data tree.
 *
 * Counts every node (leaves and interior) by the same traversal as
 * \ref cardano_uplc_data_node_ex_mem, caching the result on the node. A NULL node
 * counts 0.
 *
 * \param[in] data The data tree, or NULL.
 *
 * \return The node count.
 */
int64_t
cardano_uplc_data_node_count(const cardano_uplc_data_t* data);

/**
 * \brief Parses CBOR bytes into an arena data tree with no per-node caching.
 *
 * Decodes \p bytes as one Plutus-data item directly into arena nodes, matching the
 * library's CBOR decoding rules exactly (constr tags 121-127 / 1280-1400 / 102
 * general form, definite and indefinite maps and lists, bignum tags 2 and 3,
 * chunked indefinite byte strings) but without the per-node encoded-subtree caching
 * the library performs. Every node is served from \p arena.
 *
 * \param[in] arena The arena every node is allocated from. Must not be NULL.
 * \param[in] bytes The CBOR bytes, or NULL when \p size is 0.
 * \param[in] size The number of CBOR bytes.
 * \param[out] out On success, the decoded tree; left untouched on failure.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         \p arena or \p out is NULL, \ref CARDANO_ERROR_DECODING for malformed CBOR,
 *         or \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED if the arena or the CBOR
 *         reader cannot be allocated.
 */
cardano_error_t
cardano_uplc_data_from_cbor_bytes(
  cardano_uplc_arena_t* arena,
  const byte_t*         bytes,
  size_t                size,
  cardano_uplc_data_t** out);

/**
 * \brief Serializes an arena data tree to canonical CBOR bytes.
 *
 * Writes the tree directly to a \ref cardano_cbor_writer_t replicating the library's
 * canonical Plutus-data encoding exactly: constr tag selection (121-127, then
 * 1280-1400, then the 102 general form), non-empty lists as indefinite arrays and
 * empty lists as definite, maps definite or indefinite per the round-trip flag,
 * integers as uint / nint / bignum (tags 2 and 3) and chunked indefinite byte
 * strings at the 64-byte boundary.
 *
 * \param[in] data The data tree to serialize. Must not be NULL.
 * \param[in] writer The CBOR writer to write into. Must not be NULL.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         \p data or \p writer is NULL, or a writer error code on failure.
 */
cardano_error_t
cardano_uplc_data_to_cbor(const cardano_uplc_data_t* data, cardano_cbor_writer_t* writer);

/**
 * \brief Converts a library plutus-data object into an arena data tree.
 *
 * Recursively rebuilds \p data as arena nodes; the library object is read but not
 * referenced or retained. This is the boundary converter for parameter application
 * and the script context, which produce the heavyweight library type.
 *
 * \param[in] arena The arena every node is allocated from. Must not be NULL.
 * \param[in] data The library data object to convert. Must not be NULL.
 * \param[out] out On success, the converted tree; left untouched on failure.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         any argument is NULL, \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED if the
 *         arena cannot serve a node, or a library error code propagated from a failed
 *         accessor.
 */
cardano_error_t
cardano_uplc_data_from_plutus_data(
  cardano_uplc_arena_t*        arena,
  const cardano_plutus_data_t* data,
  cardano_uplc_data_t**        out);

/**
 * \brief Converts an arena data tree back into a refcounted library plutus-data
 *        object.
 *
 * Recursively rebuilds \p data as library nodes. The returned object is refcounted
 * and the caller owns one reference, which it must release with
 * \ref cardano_plutus_data_unref. This is the boundary converter for any consumer
 * that must hand back the library type.
 *
 * \param[in] data The arena data tree to convert. Must not be NULL.
 * \param[out] out On success, the library object; left untouched on failure.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_POINTER_IS_NULL if
 *         any argument is NULL, or a library error code propagated from a failed
 *         constructor.
 */
cardano_error_t
cardano_uplc_data_to_plutus_data(
  const cardano_uplc_data_t* data,
  cardano_plutus_data_t**    out);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BIGLUP_LABS_INCLUDE_CARDANO_UPLC_DATA_UPLC_DATA_H */
