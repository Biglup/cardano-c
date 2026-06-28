/**
 * \file uplc_data.c
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

/* INCLUDES ******************************************************************/

#include "uplc_data.h"

#include "../../allocators.h"
#include "../ast/uplc_int.h"

#include <cardano/common/byte_order.h>
#include <cardano/plutus_data/constr_plutus_data.h>
#include <cardano/plutus_data/plutus_list.h>
#include <cardano/plutus_data/plutus_map.h>

#include <src/string_safe.h>

#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* CONSTANTS *****************************************************************/

/**
 * \brief The memory units charged per Plutus-data node.
 */
// cppcheck-suppress misra-c2012-8.9; Reason: file-scope constant data grouped with the module
static const int64_t CARDANO_UPLC_DATA_NODE_COST = 4;

/**
 * \brief The number of 64-bit words an integer occupies for one memory unit.
 */
// cppcheck-suppress misra-c2012-8.9; Reason: file-scope constant data grouped with the module
static const int64_t CARDANO_UPLC_DATA_INTEGER_WORD_BITS = 64;

/**
 * \brief The byte-string chunk size: one memory unit per this many bytes.
 */
// cppcheck-suppress misra-c2012-8.9; Reason: file-scope constant data grouped with the module
static const int64_t CARDANO_UPLC_DATA_BYTE_STRING_CHUNK = 8;

/**
 * \brief The maximum Plutus-data nesting the recursive converters descend.
 *
 * The cost (ex-mem, node count), CBOR serialization and structural-equality walks are
 * iterative and carry no depth limit. Only the converters that bridge to the recursive
 * library plutus-data type (decode, and the library handoff in both directions) remain
 * recursive; this caps their recursion so adversarial nesting fails gracefully with a
 * decoding error instead of overflowing the C stack. The bound stays far above any
 * depth legitimate on-chain data reaches, and the library plutus-data type cannot
 * itself represent deeper trees, so no input the reference accepts is rejected here.
 */
static const uint32_t CARDANO_UPLC_DATA_MAX_DEPTH = 8192U;

/**
 * \brief The CBOR major-type-2 indefinite-length byte-string header byte.
 */
static const byte_t CARDANO_UPLC_DATA_INDEFINITE_BYTE_STRING = 95;

/**
 * \brief The byte-string chunk size used by the canonical CBOR serializer.
 */
static const size_t CARDANO_UPLC_DATA_CBOR_CHUNK = 64U;

/**
 * \brief Marks an uncomputed memoized field on a data node.
 */
static const int64_t CARDANO_UPLC_DATA_UNCOMPUTED = -1;

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Releases the arena's reference to a bigint registered for unref.
 *
 * \param[in] object The \ref cardano_bigint_t the arena owns a reference to.
 */
static void
unref_bigint(void* object)
{
  cardano_bigint_t* bigint = (cardano_bigint_t*)object;

  cardano_bigint_unref(&bigint);
}

/**
 * \brief A pair of nodes pending a structural-equality comparison.
 */
typedef struct equals_pair_t
{
    const cardano_uplc_data_t* lhs;
    const cardano_uplc_data_t* rhs;
} equals_pair_t;

/**
 * \brief Grows a heap work stack to hold at least one more element.
 *
 * \param[in,out] stack The stack base pointer.
 * \param[in,out] capacity The current element capacity.
 * \param[in] element_size The size in bytes of one element.
 *
 * \return \c true on success, \c false if reallocation fails.
 */
static bool
stack_reserve(void** stack, size_t* capacity, size_t element_size)
{
  size_t next  = (*capacity == 0U) ? 64U : (*capacity * 2U);
  void*  grown = _cardano_realloc(*stack, next * element_size);

  if (grown == NULL)
  {
    return false;
  }

  *stack    = grown;
  *capacity = next;

  return true;
}

/**
 * \brief Pushes a node pair onto the equality work stack, growing it as needed.
 *
 * \param[in,out] stack The stack base pointer.
 * \param[in,out] capacity The current element capacity.
 * \param[in,out] count The current element count.
 * \param[in] lhs The first node of the pair.
 * \param[in] rhs The second node of the pair.
 *
 * \return \c true on success, \c false if the stack cannot grow.
 */
static bool
equals_push(
  equals_pair_t**            stack,
  size_t*                    capacity,
  size_t*                    count,
  const cardano_uplc_data_t* lhs,
  const cardano_uplc_data_t* rhs)
{
  if ((*count >= *capacity) && !stack_reserve((void**)stack, capacity, sizeof(equals_pair_t)))
  {
    return false;
  }

  (*stack)[*count].lhs = lhs;
  (*stack)[*count].rhs = rhs;
  ++(*count);

  return true;
}

/**
 * \brief A post-order traversal frame: a node and whether its children were pushed.
 */
typedef struct walk_frame_t
{
    const cardano_uplc_data_t* node;
    bool                       expanded;
} walk_frame_t;

/**
 * \brief Pushes a traversal frame onto a heap work stack, growing it as needed.
 *
 * \param[in,out] stack The stack base pointer.
 * \param[in,out] capacity The current element capacity.
 * \param[in,out] count The current element count.
 * \param[in] node The node to push.
 * \param[in] expanded Whether the frame's children have already been pushed.
 *
 * \return \c true on success, \c false if the stack cannot grow.
 */
static bool
walk_push(
  walk_frame_t**             stack,
  size_t*                    capacity,
  size_t*                    count,
  const cardano_uplc_data_t* node,
  bool                       expanded)
{
  if ((*count >= *capacity) && !stack_reserve((void**)stack, capacity, sizeof(walk_frame_t)))
  {
    return false;
  }

  (*stack)[*count].node     = node;
  (*stack)[*count].expanded = expanded;
  ++(*count);

  return true;
}

/**
 * \brief Allocates and zero-initializes a bare data node from the arena.
 *
 * Sets the memoized ex-mem and node-count fields to the uncomputed sentinel and
 * leaves the union arm to the caller.
 *
 * \param[in] arena The arena to allocate from.
 *
 * \return The new node, or NULL when the arena cannot serve it.
 */
static cardano_uplc_data_t*
alloc_node(cardano_uplc_arena_t* arena)
{
  cardano_uplc_data_t* node = (cardano_uplc_data_t*)cardano_uplc_arena_alloc(arena, sizeof(cardano_uplc_data_t), 0U);

  if (node == NULL)
  {
    return NULL;
  }

  (void)memset(node, 0, sizeof(cardano_uplc_data_t));

  node->ex_mem     = CARDANO_UPLC_DATA_UNCOMPUTED;
  node->node_count = CARDANO_UPLC_DATA_UNCOMPUTED;

  return node;
}

/**
 * \brief Returns the ex-mem of an integer leaf in 64-bit words.
 *
 * Zero costs one word; a non-zero integer costs floor(log2(|n|)) / 64 + 1.
 *
 * \param[in] data An integer data node.
 *
 * \return The integer leaf ex-mem.
 */
static int64_t
integer_ex_mem(const cardano_uplc_data_t* data)
{
  size_t  bits = 0U;
  int64_t log2 = 0;

  if (data->as.integer.is_small)
  {
    uint64_t magnitude = 0U;

    if (data->as.integer.small == 0)
    {
      return 1;
    }

    if (data->as.integer.small < 0)
    {
      magnitude = (uint64_t)(-(data->as.integer.small + 1)) + 1U;
    }
    else
    {
      magnitude = (uint64_t)data->as.integer.small;
    }

    bits = 0U;

    while (magnitude != 0U)
    {
      ++bits;
      magnitude >>= 1U;
    }

    log2 = (int64_t)bits - 1;

    return (log2 / CARDANO_UPLC_DATA_INTEGER_WORD_BITS) + 1;
  }

  if ((data->as.integer.big == NULL) || cardano_bigint_is_zero(data->as.integer.big))
  {
    return 1;
  }

  bits = cardano_bigint_bit_length(data->as.integer.big);
  log2 = (int64_t)bits - 1;

  return (log2 / CARDANO_UPLC_DATA_INTEGER_WORD_BITS) + 1;
}

/**
 * \brief Returns the ex-mem of a byte-string leaf in 8-byte chunks.
 *
 * \param[in] length The byte-string length.
 *
 * \return The byte-string leaf ex-mem.
 */
static int64_t
byte_string_ex_mem(size_t length)
{
  if (length == 0U)
  {
    return 1;
  }

  return (((int64_t)length - 1) / CARDANO_UPLC_DATA_BYTE_STRING_CHUNK) + 1;
}

/**
 * \brief Computes the data ex-mem of a subtree, depth-bounded.
 *
 * \param[in] data The data node, or NULL.
 * \param[in] depth The current recursion depth.
 *
 * \return The subtree ex-mem.
 */
/**
 * \brief Pushes the immediate children of a node onto a node work stack.
 *
 * \param[in] data The parent node.
 * \param[in,out] stack The stack base pointer.
 * \param[in,out] capacity The current element capacity.
 * \param[in,out] count The current element count.
 *
 * \return \c true on success, \c false if the stack cannot grow.
 */
static bool
push_children(
  const cardano_uplc_data_t* data,
  walk_frame_t**             stack,
  size_t*                    capacity,
  size_t*                    count)
{
  size_t i = 0U;

  switch (data->kind)
  {
    case CARDANO_UPLC_DATA_KIND_CONSTR:
    {
      for (i = 0U; i < data->as.constr.count; ++i)
      {
        if (!walk_push(stack, capacity, count, data->as.constr.fields[i], false))
        {
          return false;
        }
      }

      break;
    }
    case CARDANO_UPLC_DATA_KIND_MAP:
    {
      for (i = 0U; i < data->as.map.count; ++i)
      {
        if (!walk_push(stack, capacity, count, data->as.map.entries[i].key, false) || !walk_push(stack, capacity, count, data->as.map.entries[i].value, false))
        {
          return false;
        }
      }

      break;
    }
    case CARDANO_UPLC_DATA_KIND_LIST:
    {
      for (i = 0U; i < data->as.list.count; ++i)
      {
        if (!walk_push(stack, capacity, count, data->as.list.items[i], false))
        {
          return false;
        }
      }

      break;
    }
    case CARDANO_UPLC_DATA_KIND_INTEGER:
    case CARDANO_UPLC_DATA_KIND_BYTES:
    default:
    {
      break;
    }
  }

  return true;
}

/**
 * \brief Sums the memoized ex-mem of a node's immediate children.
 *
 * \param[in] data The parent node, whose children's ex-mem memos are set.
 *
 * \return The total child ex-mem.
 */
static int64_t
children_ex_mem(const cardano_uplc_data_t* data)
{
  int64_t total = 0;
  size_t  i     = 0U;

  switch (data->kind)
  {
    case CARDANO_UPLC_DATA_KIND_CONSTR:
    {
      for (i = 0U; i < data->as.constr.count; ++i)
      {
        const cardano_uplc_data_t* child = data->as.constr.fields[i];

        total += (child != NULL) ? child->ex_mem : 0;
      }

      break;
    }
    case CARDANO_UPLC_DATA_KIND_MAP:
    {
      for (i = 0U; i < data->as.map.count; ++i)
      {
        const cardano_uplc_data_t* key   = data->as.map.entries[i].key;
        const cardano_uplc_data_t* value = data->as.map.entries[i].value;

        total += (key != NULL) ? key->ex_mem : 0;
        total += (value != NULL) ? value->ex_mem : 0;
      }

      break;
    }
    case CARDANO_UPLC_DATA_KIND_LIST:
    {
      for (i = 0U; i < data->as.list.count; ++i)
      {
        const cardano_uplc_data_t* child = data->as.list.items[i];

        total += (child != NULL) ? child->ex_mem : 0;
      }

      break;
    }
    case CARDANO_UPLC_DATA_KIND_INTEGER:
    {
      total += integer_ex_mem(data);

      break;
    }
    case CARDANO_UPLC_DATA_KIND_BYTES:
    {
      total += byte_string_ex_mem(data->as.bytes.size);

      break;
    }
    default:
    {
      break;
    }
  }

  return total;
}

/**
 * \brief Sums the memoized node count of a node's immediate children.
 *
 * \param[in] data The parent node, whose children's node-count memos are set.
 *
 * \return The total child node count.
 */
static int64_t
children_node_count(const cardano_uplc_data_t* data)
{
  int64_t total = 0;
  size_t  i     = 0U;

  switch (data->kind)
  {
    case CARDANO_UPLC_DATA_KIND_CONSTR:
    {
      for (i = 0U; i < data->as.constr.count; ++i)
      {
        const cardano_uplc_data_t* child = data->as.constr.fields[i];

        total += (child != NULL) ? child->node_count : 0;
      }

      break;
    }
    case CARDANO_UPLC_DATA_KIND_MAP:
    {
      for (i = 0U; i < data->as.map.count; ++i)
      {
        const cardano_uplc_data_t* key   = data->as.map.entries[i].key;
        const cardano_uplc_data_t* value = data->as.map.entries[i].value;

        total += (key != NULL) ? key->node_count : 0;
        total += (value != NULL) ? value->node_count : 0;
      }

      break;
    }
    case CARDANO_UPLC_DATA_KIND_LIST:
    {
      for (i = 0U; i < data->as.list.count; ++i)
      {
        const cardano_uplc_data_t* child = data->as.list.items[i];

        total += (child != NULL) ? child->node_count : 0;
      }

      break;
    }
    case CARDANO_UPLC_DATA_KIND_INTEGER:
    case CARDANO_UPLC_DATA_KIND_BYTES:
    default:
    {
      break;
    }
  }

  return total;
}

/**
 * \brief Computes the data ex-mem of a subtree iteratively, memoizing every node.
 *
 * Uses an explicit post-order work stack so adversarial nesting cannot overflow the C
 * stack. On allocation failure the partial walk is abandoned and the contribution of
 * the unmeasured subtree is taken as the per-node base cost so the result is never
 * larger than the true ex-mem.
 *
 * \param[in] data The data node, or NULL.
 *
 * \return The subtree ex-mem.
 */
static int64_t
compute_ex_mem(const cardano_uplc_data_t* data)
{
  walk_frame_t* stack    = NULL;
  size_t        capacity = 0U;
  size_t        count    = 0U;

  if (data == NULL)
  {
    return 0;
  }

  if (data->ex_mem != CARDANO_UPLC_DATA_UNCOMPUTED)
  {
    return data->ex_mem;
  }

  if (!walk_push(&stack, &capacity, &count, data, false))
  {
    return CARDANO_UPLC_DATA_NODE_COST;
  }

  while (count > 0U)
  {
    // cppcheck-suppress misra-c2012-13.3; Reason: local post-increment with no aliasing
    walk_frame_t frame = stack[--count];

    if ((frame.node == NULL) || (frame.node->ex_mem != CARDANO_UPLC_DATA_UNCOMPUTED))
    {
      continue;
    }

    if (frame.expanded)
    {
      // cppcheck-suppress misra-c2012-11.8; Reason: interfacing a non-const-correct API
      ((cardano_uplc_data_t*)((void*)frame.node))->ex_mem = CARDANO_UPLC_DATA_NODE_COST + children_ex_mem(frame.node);

      continue;
    }

    if (!walk_push(&stack, &capacity, &count, frame.node, true) || !push_children(frame.node, &stack, &capacity, &count))
    {
      _cardano_free(stack);

      return CARDANO_UPLC_DATA_NODE_COST;
    }
  }

  _cardano_free(stack);

  return data->ex_mem;
}

/**
 * \brief Counts the nodes of a subtree iteratively, memoizing every node.
 *
 * Uses an explicit post-order work stack so adversarial nesting cannot overflow the C
 * stack. On allocation failure the partial walk is abandoned and the contribution of
 * the uncounted subtree is taken as a single node so the result is never larger than
 * the true node count.
 *
 * \param[in] data The data node, or NULL.
 *
 * \return The subtree node count.
 */
static int64_t
compute_node_count(const cardano_uplc_data_t* data)
{
  walk_frame_t* stack    = NULL;
  size_t        capacity = 0U;
  size_t        count    = 0U;

  if (data == NULL)
  {
    return 0;
  }

  if (data->node_count != CARDANO_UPLC_DATA_UNCOMPUTED)
  {
    return data->node_count;
  }

  if (!walk_push(&stack, &capacity, &count, data, false))
  {
    return 1;
  }

  while (count > 0U)
  {
    // cppcheck-suppress misra-c2012-13.3; Reason: local post-increment with no aliasing
    walk_frame_t frame = stack[--count];

    if ((frame.node == NULL) || (frame.node->node_count != CARDANO_UPLC_DATA_UNCOMPUTED))
    {
      continue;
    }

    if (frame.expanded)
    {
      // cppcheck-suppress misra-c2012-11.8; Reason: interfacing a non-const-correct API
      ((cardano_uplc_data_t*)((void*)frame.node))->node_count = 1 + children_node_count(frame.node);

      continue;
    }

    if (!walk_push(&stack, &capacity, &count, frame.node, true) || !push_children(frame.node, &stack, &capacity, &count))
    {
      _cardano_free(stack);

      return 1;
    }
  }

  _cardano_free(stack);

  return data->node_count;
}

/**
 * \brief Tests two integer data nodes for value equality.
 *
 * Compares the inline values directly when both are inline; otherwise materializes
 * is avoided by comparing through the bigint when present and the inline value
 * promoted to a bigint-free comparison when one side is inline and equal sign.
 *
 * \param[in] lhs The first integer node.
 * \param[in] rhs The second integer node.
 *
 * \return \c true when equal, \c false otherwise.
 */
static bool
integer_equals(const cardano_uplc_data_t* lhs, const cardano_uplc_data_t* rhs)
{
  if (lhs->as.integer.is_small && rhs->as.integer.is_small)
  {
    return lhs->as.integer.small == rhs->as.integer.small;
  }

  if (!lhs->as.integer.is_small && !rhs->as.integer.is_small)
  {
    return cardano_bigint_equals(lhs->as.integer.big, rhs->as.integer.big);
  }

  {
    const cardano_uplc_data_t* small_side = lhs->as.integer.is_small ? lhs : rhs;
    const cardano_uplc_data_t* big_side   = lhs->as.integer.is_small ? rhs : lhs;
    int64_t                    big_inline = 0;

    if (cardano_uplc_int_bigint_fits_int64(big_side->as.integer.big, &big_inline))
    {
      return big_inline == small_side->as.integer.small;
    }
  }

  return false;
}

/**
 * \brief Writes the canonical CBOR for an integer leaf.
 *
 * Replicates the library's plutus-data integer encoding exactly: uint / nint for
 * values fitting 64 bits and the bignum tags 2 / 3 with chunked indefinite byte
 * strings for larger magnitudes.
 *
 * \param[in] data An integer data node.
 * \param[in] writer The CBOR writer.
 *
 * \return \ref CARDANO_SUCCESS on success, or a writer or bigint error code.
 */
static cardano_error_t
write_integer(const cardano_uplc_data_t* data, cardano_cbor_writer_t* writer)
{
  if (data->as.integer.is_small)
  {
    if (data->as.integer.small < 0)
    {
      return cardano_cbor_writer_write_signed_int(writer, data->as.integer.small);
    }

    return cardano_cbor_writer_write_uint(writer, (uint64_t)data->as.integer.small);
  }

  {
    const cardano_bigint_t* value      = data->as.integer.big;
    size_t                  bit_length = cardano_bigint_bit_length(value);
    cardano_error_t         result     = CARDANO_SUCCESS;

    if ((cardano_bigint_signum(value) < 0) && (bit_length <= 64U))
    {
      return cardano_cbor_writer_write_signed_int(writer, cardano_bigint_to_int(value));
    }

    if (bit_length <= 64U)
    {
      return cardano_cbor_writer_write_uint(writer, cardano_bigint_to_unsigned_int(value));
    }

    {
      const size_t size  = cardano_bigint_get_bytes_size(value);
      byte_t*      bytes = (byte_t*)_cardano_malloc(size);

      if (bytes == NULL)
      {
        return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
      }

      result = cardano_bigint_to_bytes(value, CARDANO_BYTE_ORDER_BIG_ENDIAN, bytes, size);

      if (result == CARDANO_SUCCESS)
      {
        result = cardano_cbor_writer_write_tag(
          writer,
          (cardano_bigint_signum(value) < 0) ? CARDANO_CBOR_TAG_NEGATIVE_BIG_NUM : CARDANO_CBOR_TAG_UNSIGNED_BIG_NUM);
      }

      if (result == CARDANO_SUCCESS)
      {
        if (size <= CARDANO_UPLC_DATA_CBOR_CHUNK)
        {
          result = cardano_cbor_writer_write_bytestring(writer, bytes, size);
        }
        else
        {
          const size_t chunks = size / CARDANO_UPLC_DATA_CBOR_CHUNK;
          const size_t rest   = size % CARDANO_UPLC_DATA_CBOR_CHUNK;
          size_t       i      = 0U;

          result = cardano_cbor_writer_write_encoded(writer, &CARDANO_UPLC_DATA_INDEFINITE_BYTE_STRING, sizeof(CARDANO_UPLC_DATA_INDEFINITE_BYTE_STRING));

          for (i = 0U; (i < chunks) && (result == CARDANO_SUCCESS); ++i)
          {
            result = cardano_cbor_writer_write_bytestring(writer, &bytes[i * CARDANO_UPLC_DATA_CBOR_CHUNK], CARDANO_UPLC_DATA_CBOR_CHUNK);
          }

          if ((result == CARDANO_SUCCESS) && (rest > 0U))
          {
            result = cardano_cbor_writer_write_bytestring(writer, &bytes[chunks * CARDANO_UPLC_DATA_CBOR_CHUNK], rest);
          }

          if (result == CARDANO_SUCCESS)
          {
            result = cardano_cbor_writer_write_end_array(writer);
          }
        }
      }

      _cardano_free(bytes);

      return result;
    }
  }
}

/**
 * \brief Writes the canonical CBOR for a byte-string leaf.
 *
 * \param[in] data A byte-string data node.
 * \param[in] writer The CBOR writer.
 *
 * \return \ref CARDANO_SUCCESS on success, or a writer error code.
 */
static cardano_error_t
write_bytes(const cardano_uplc_data_t* data, cardano_cbor_writer_t* writer)
{
  static const byte_t empty = 0U;
  const size_t        size  = data->as.bytes.size;
  const byte_t*       bytes = (data->as.bytes.data != NULL) ? data->as.bytes.data : &empty;

  if (size <= CARDANO_UPLC_DATA_CBOR_CHUNK)
  {
    return cardano_cbor_writer_write_bytestring(writer, bytes, size);
  }

  {
    const size_t    chunks = size / CARDANO_UPLC_DATA_CBOR_CHUNK;
    const size_t    rest   = size % CARDANO_UPLC_DATA_CBOR_CHUNK;
    size_t          i      = 0U;
    cardano_error_t result = cardano_cbor_writer_write_encoded(writer, &CARDANO_UPLC_DATA_INDEFINITE_BYTE_STRING, sizeof(CARDANO_UPLC_DATA_INDEFINITE_BYTE_STRING));

    for (i = 0U; (i < chunks) && (result == CARDANO_SUCCESS); ++i)
    {
      result = cardano_cbor_writer_write_bytestring(writer, &bytes[i * CARDANO_UPLC_DATA_CBOR_CHUNK], CARDANO_UPLC_DATA_CBOR_CHUNK);
    }

    if ((result == CARDANO_SUCCESS) && (rest > 0U))
    {
      result = cardano_cbor_writer_write_bytestring(writer, &bytes[chunks * CARDANO_UPLC_DATA_CBOR_CHUNK], rest);
    }

    if (result == CARDANO_SUCCESS)
    {
      result = cardano_cbor_writer_write_end_array(writer);
    }

    return result;
  }
}

/**
 * \brief Maps a constructor alternative to its canonical compact CBOR tag.
 *
 * \param[in] alternative The constructor alternative.
 *
 * \return The compact tag 121-127, the ranged tag 1280-1400, or 102 for the general
 *         form.
 */
static uint64_t
alternative_to_tag(uint64_t alternative)
{
  if (alternative <= 6U)
  {
    return 121U + alternative;
  }

  if (alternative <= 127U)
  {
    return ((uint64_t)1280U - (uint64_t)7U) + alternative;
  }

  return 102U;
}

/**
 * \brief Writes the opening CBOR of a node and pushes its children in walk order.
 *
 * For a container this writes the tag/header and start markers and pushes a deferred
 * close frame plus every child in reverse so the children emit front-to-back. For a
 * leaf it writes the whole encoding and pushes nothing.
 *
 * \param[in] data The data node.
 * \param[in] writer The CBOR writer.
 * \param[in,out] stack The traversal work stack.
 * \param[in,out] capacity The current stack capacity.
 * \param[in,out] count The current stack count.
 *
 * \return \ref CARDANO_SUCCESS on success, or an error code.
 */
static cardano_error_t
to_cbor_open(
  const cardano_uplc_data_t* data,
  cardano_cbor_writer_t*     writer,
  walk_frame_t**             stack,
  size_t*                    capacity,
  size_t*                    count)
{
  cardano_error_t result = CARDANO_SUCCESS;
  size_t          i      = 0U;

  switch (data->kind)
  {
    case CARDANO_UPLC_DATA_KIND_CONSTR:
    {
      uint64_t tag = alternative_to_tag(data->as.constr.tag);

      result = cardano_cbor_writer_write_tag(writer, (cardano_cbor_tag_t)tag);

      if ((result == CARDANO_SUCCESS) && (tag == 102U))
      {
        result = cardano_cbor_writer_write_start_array(writer, 2);

        if (result == CARDANO_SUCCESS)
        {
          result = cardano_cbor_writer_write_uint(writer, data->as.constr.tag);
        }
      }

      if (result == CARDANO_SUCCESS)
      {
        result = cardano_cbor_writer_write_start_array(writer, (data->as.constr.count > 0U) ? -1 : 0);
      }

      if ((result == CARDANO_SUCCESS) && !walk_push(stack, capacity, count, data, true))
      {
        return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
      }

      for (i = data->as.constr.count; (result == CARDANO_SUCCESS) && (i > 0U); --i)
      {
        if (!walk_push(stack, capacity, count, data->as.constr.fields[i - 1U], false))
        {
          return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
        }
      }

      break;
    }
    case CARDANO_UPLC_DATA_KIND_MAP:
    {
      if (data->as.map.indefinite)
      {
        result = cardano_cbor_writer_write_start_map(writer, -1);
      }
      else
      {
        result = cardano_cbor_writer_write_start_map(writer, (int64_t)data->as.map.count);
      }

      if ((result == CARDANO_SUCCESS) && !walk_push(stack, capacity, count, data, true))
      {
        return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
      }

      for (i = data->as.map.count; (result == CARDANO_SUCCESS) && (i > 0U); --i)
      {
        if (!walk_push(stack, capacity, count, data->as.map.entries[i - 1U].value, false) || !walk_push(stack, capacity, count, data->as.map.entries[i - 1U].key, false))
        {
          return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
        }
      }

      break;
    }
    case CARDANO_UPLC_DATA_KIND_LIST:
    {
      result = cardano_cbor_writer_write_start_array(writer, (data->as.list.count > 0U) ? -1 : 0);

      if ((result == CARDANO_SUCCESS) && !walk_push(stack, capacity, count, data, true))
      {
        return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
      }

      for (i = data->as.list.count; (result == CARDANO_SUCCESS) && (i > 0U); --i)
      {
        if (!walk_push(stack, capacity, count, data->as.list.items[i - 1U], false))
        {
          return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
        }
      }

      break;
    }
    case CARDANO_UPLC_DATA_KIND_INTEGER:
    {
      result = write_integer(data, writer);

      break;
    }
    case CARDANO_UPLC_DATA_KIND_BYTES:
    {
      result = write_bytes(data, writer);

      break;
    }
    default:
    {
      result = CARDANO_ERROR_INVALID_ARGUMENT;

      break;
    }
  }

  return result;
}

/**
 * \brief Writes the closing CBOR marker of a container node, if any.
 *
 * \param[in] data The container node.
 * \param[in] writer The CBOR writer.
 *
 * \return \ref CARDANO_SUCCESS on success, or an error code.
 */
static cardano_error_t
to_cbor_close(const cardano_uplc_data_t* data, cardano_cbor_writer_t* writer)
{
  switch (data->kind)
  {
    case CARDANO_UPLC_DATA_KIND_CONSTR:
    {
      if (data->as.constr.count > 0U)
      {
        return cardano_cbor_writer_write_end_array(writer);
      }

      break;
    }
    case CARDANO_UPLC_DATA_KIND_MAP:
    {
      if (data->as.map.indefinite)
      {
        return cardano_cbor_writer_write_end_map(writer);
      }

      break;
    }
    case CARDANO_UPLC_DATA_KIND_LIST:
    {
      if (data->as.list.count > 0U)
      {
        return cardano_cbor_writer_write_end_array(writer);
      }

      break;
    }
    case CARDANO_UPLC_DATA_KIND_INTEGER:
    case CARDANO_UPLC_DATA_KIND_BYTES:
    default:
    {
      break;
    }
  }

  return CARDANO_SUCCESS;
}

/**
 * \brief Iterative CBOR serializer for an arena data subtree.
 *
 * Walks the tree with an explicit work stack so adversarial nesting cannot overflow
 * the C stack. The byte stream is identical to a recursive pre-order emit: each
 * container writes its header and start markers, then its children in order, then its
 * close marker.
 *
 * \param[in] data The data node.
 * \param[in] writer The CBOR writer.
 *
 * \return \ref CARDANO_SUCCESS on success, or an error code.
 */
static cardano_error_t
to_cbor(const cardano_uplc_data_t* data, cardano_cbor_writer_t* writer)
{
  walk_frame_t*   stack    = NULL;
  size_t          capacity = 0U;
  size_t          count    = 0U;
  cardano_error_t result   = CARDANO_SUCCESS;

  if (data == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (!walk_push(&stack, &capacity, &count, data, false))
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  while ((result == CARDANO_SUCCESS) && (count > 0U))
  {
    // cppcheck-suppress misra-c2012-13.3; Reason: local post-increment with no aliasing
    walk_frame_t frame = stack[--count];

    if (frame.node == NULL)
    {
      result = CARDANO_ERROR_POINTER_IS_NULL;

      break;
    }

    if (frame.expanded)
    {
      result = to_cbor_close(frame.node, writer);
    }
    else
    {
      result = to_cbor_open(frame.node, writer, &stack, &capacity, &count);
    }
  }

  _cardano_free(stack);

  return result;
}

/**
 * \brief The CBOR major type carried in the top three bits of a head byte.
 */
typedef enum
{
  /** \brief Major type 0: an unsigned integer. */
  PRV_CBOR_MAJOR_UNSIGNED = 0,
  /** \brief Major type 1: a negative integer. */
  PRV_CBOR_MAJOR_NEGATIVE = 1,
  /** \brief Major type 2: a byte string. */
  PRV_CBOR_MAJOR_BYTES = 2,
  /** \brief Major type 4: an array. */
  PRV_CBOR_MAJOR_ARRAY = 4,
  /** \brief Major type 5: a map. */
  PRV_CBOR_MAJOR_MAP = 5,
  /** \brief Major type 6: a semantic tag. */
  PRV_CBOR_MAJOR_TAG = 6
} cbor_major_t;

/**
 * \brief The additional-information field carried in the low five bits of a head byte.
 */
typedef enum
{
  /** \brief Argument follows in one byte. */
  PRV_CBOR_INFO_ONE_BYTE = 24,
  /** \brief Argument follows in two big-endian bytes. */
  PRV_CBOR_INFO_TWO_BYTES = 25,
  /** \brief Argument follows in four big-endian bytes. */
  PRV_CBOR_INFO_FOUR_BYTES = 26,
  /** \brief Argument follows in eight big-endian bytes. */
  PRV_CBOR_INFO_EIGHT_BYTES = 27,
  /** \brief The item is indefinite-length. */
  PRV_CBOR_INFO_INDEFINITE = 31
} cbor_info_t;

/**
 * \brief The byte-string chunk size the canonical serializer emits, repeated here so
 *        the parser uses the same named bound.
 */
static const uint64_t PRV_CBOR_BREAK = 0xFFU;

/**
 * \brief The CBOR tag selecting the constructor general form.
 */
// cppcheck-suppress misra-c2012-8.9; Reason: file-scope constant data grouped with the module
static const uint64_t PRV_CONSTR_GENERAL_FORM_TAG = 102U;

/**
 * \brief The first CBOR tag of the compact constructor range (alternative 0).
 */
// cppcheck-suppress misra-c2012-8.9; Reason: file-scope constant data grouped with the module
static const uint64_t PRV_CONSTR_COMPACT_TAG_LO = 121U;

/**
 * \brief The last CBOR tag of the compact constructor range (alternative 6).
 */
// cppcheck-suppress misra-c2012-8.9; Reason: file-scope constant data grouped with the module
static const uint64_t PRV_CONSTR_COMPACT_TAG_HI = 127U;

/**
 * \brief The first CBOR tag of the ranged constructor form (alternative 7).
 */
// cppcheck-suppress misra-c2012-8.9; Reason: file-scope constant data grouped with the module
static const uint64_t PRV_CONSTR_RANGED_TAG_LO = 1280U;

/**
 * \brief The last CBOR tag of the ranged constructor form (alternative 127).
 */
// cppcheck-suppress misra-c2012-8.9; Reason: file-scope constant data grouped with the module
static const uint64_t PRV_CONSTR_RANGED_TAG_HI = 1400U;

/**
 * \brief The alternative the first ranged-form tag maps to.
 */
// cppcheck-suppress misra-c2012-8.9; Reason: file-scope constant data grouped with the module
static const uint64_t PRV_CONSTR_RANGED_OFFSET = 7U;

/**
 * \brief A forward, allocation-free cursor over the CBOR byte buffer.
 *
 * The decoder threads a single \ref cursor_t through the recursion: it never
 * slices the buffer, never refcounts, and advances \c offset in place. Bytes for a
 * byte-string node are copied straight into the arena from the cursor.
 */
typedef struct cursor_t
{
    const byte_t* buf;
    size_t        size;
    size_t        offset;
} cursor_t;

/**
 * \brief Reads one head byte from the cursor, splitting it into major type and
 *        additional information.
 *
 * \param[in,out] cursor The byte cursor; advanced one byte on success.
 * \param[out] major On success, the major type (top three bits).
 * \param[out] info On success, the additional information (low five bits).
 *
 * \return \ref CARDANO_SUCCESS on success, or \ref CARDANO_ERROR_DECODING when the
 *         cursor is exhausted.
 */
static cardano_error_t
read_head(cursor_t* cursor, byte_t* major, byte_t* info)
{
  byte_t head = 0U;

  if (cursor->offset >= cursor->size)
  {
    return CARDANO_ERROR_DECODING;
  }

  head = cursor->buf[cursor->offset];
  ++cursor->offset;

  *major = (byte_t)((head >> 5U) & 0x07U);
  *info  = (byte_t)(head & 0x1FU);

  return CARDANO_SUCCESS;
}

/**
 * \brief Reads the unsigned argument that follows a head byte for the given
 *        additional information.
 *
 * Mirrors \c _cbor_reader_decode_unsigned_integer: \c info below 24 is the immediate
 * value, 24/25/26/27 read 1/2/4/8 following big-endian bytes, and anything else (28-31)
 * is rejected. No canonical-minimal-encoding check is performed, matching the library.
 *
 * \param[in,out] cursor The byte cursor; advanced past the argument on success.
 * \param[in] info The additional information from the head byte.
 * \param[out] out On success, the decoded unsigned argument.
 *
 * \return \ref CARDANO_SUCCESS on success, or \ref CARDANO_ERROR_DECODING on a
 *         truncated or malformed argument.
 */
static cardano_error_t
read_argument(cursor_t* cursor, byte_t info, uint64_t* out)
{
  size_t   width = 0U;
  uint64_t value = 0U;
  size_t   i     = 0U;

  if (info < (byte_t)PRV_CBOR_INFO_ONE_BYTE)
  {
    *out = (uint64_t)info;

    return CARDANO_SUCCESS;
  }

  switch (info)
  {
    case PRV_CBOR_INFO_ONE_BYTE:
    {
      width = 1U;

      break;
    }
    case PRV_CBOR_INFO_TWO_BYTES:
    {
      width = 2U;

      break;
    }
    case PRV_CBOR_INFO_FOUR_BYTES:
    {
      width = 4U;

      break;
    }
    case PRV_CBOR_INFO_EIGHT_BYTES:
    {
      width = 8U;

      break;
    }
    default:
    {
      return CARDANO_ERROR_DECODING;
    }
  }

  if ((cursor->offset + width) > cursor->size)
  {
    return CARDANO_ERROR_DECODING;
  }

  for (i = 0U; i < width; ++i)
  {
    value = (value << 8U) | (uint64_t)cursor->buf[cursor->offset + i];
  }

  cursor->offset += width;
  *out           = value;

  return CARDANO_SUCCESS;
}

/**
 * \brief Reads one data item from the cursor into a freshly allocated arena node.
 *
 * \param[in] arena The arena every node is allocated from.
 * \param[in,out] cursor The byte cursor; advanced past the item on success.
 * \param[in] depth The current recursion depth.
 * \param[out] out On success, the parsed node.
 *
 * \return \ref CARDANO_SUCCESS on success, \ref CARDANO_ERROR_DECODING for malformed
 *         CBOR, or \ref CARDANO_ERROR_MEMORY_ALLOCATION_FAILED on arena exhaustion.
 */
static cardano_error_t
parse_data_node(cardano_uplc_arena_t* arena, cursor_t* cursor, uint32_t depth, cardano_uplc_data_t** out);

/**
 * \brief Builds a bignum integer node from an owned bigint.
 *
 * \param[in] arena The arena.
 * \param[in] integer The bigint just built; ownership transfers to this function.
 * \param[out] out On success, the integer node.
 *
 * \return \ref CARDANO_SUCCESS on success, or an error code. On every exit the bigint
 *         is released.
 */
static cardano_error_t
node_from_bigint(cardano_uplc_arena_t* arena, cardano_bigint_t* integer, cardano_uplc_data_t** out)
{
  cardano_error_t result = cardano_uplc_data_new_integer(arena, integer, out);

  cardano_bigint_unref(&integer);

  return result;
}

/**
 * \brief Reads a byte-string item (definite or indefinite/chunked) into an arena
 *        byte node.
 *
 * For a definite string the bytes are copied straight from the cursor. For the
 * indefinite form (head byte 0x5F) the chunk byte strings are concatenated in order,
 * exactly as \c _cbor_reader_read_indefinite_length_concatenated does; only definite
 * byte-string chunks are accepted and the run ends at the 0xFF break.
 *
 * \param[in] arena The arena.
 * \param[in,out] cursor The byte cursor, positioned just past the head byte.
 * \param[in] info The additional information from the head byte.
 * \param[out] out On success, the byte node.
 *
 * \return \ref CARDANO_SUCCESS on success, or an error code.
 */
static cardano_error_t
parse_bytes(cardano_uplc_arena_t* arena, cursor_t* cursor, byte_t info, cardano_uplc_data_t** out)
{
  uint64_t        length = 0U;
  cardano_error_t result;

  if (info != (byte_t)PRV_CBOR_INFO_INDEFINITE)
  {
    const byte_t* data = NULL;

    result = read_argument(cursor, info, &length);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    if ((length > (uint64_t)(cursor->size - cursor->offset)) || (length > (uint64_t)SIZE_MAX))
    {
      return CARDANO_ERROR_DECODING;
    }

    data           = &cursor->buf[cursor->offset];
    cursor->offset += (size_t)length;

    return cardano_uplc_data_new_bytes(arena, data, (size_t)length, out);
  }

  {
    byte_t* scratch  = NULL;
    size_t  total    = 0U;
    size_t  capacity = 0U;

    for (;;)
    {
      byte_t   chunk_major = 0U;
      byte_t   chunk_info  = 0U;
      uint64_t chunk_len   = 0U;

      if (cursor->offset >= cursor->size)
      {
        return CARDANO_ERROR_DECODING;
      }

      if (cursor->buf[cursor->offset] == (byte_t)PRV_CBOR_BREAK)
      {
        ++cursor->offset;

        break;
      }

      result = read_head(cursor, &chunk_major, &chunk_info);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      if ((chunk_major != (byte_t)PRV_CBOR_MAJOR_BYTES) || (chunk_info == (byte_t)PRV_CBOR_INFO_INDEFINITE))
      {
        return CARDANO_ERROR_DECODING;
      }

      result = read_argument(cursor, chunk_info, &chunk_len);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      if (chunk_len > (uint64_t)(cursor->size - cursor->offset))
      {
        return CARDANO_ERROR_DECODING;
      }

      if ((total + (size_t)chunk_len) > capacity)
      {
        size_t  new_capacity = ((capacity == 0U) ? 128U : (capacity * 2U));
        byte_t* grown        = NULL;

        while (new_capacity < (total + (size_t)chunk_len))
        {
          new_capacity *= 2U;
        }

        grown = (byte_t*)cardano_uplc_arena_alloc(arena, new_capacity, 1U);

        if (grown == NULL)
        {
          return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
        }

        if (total > 0U)
        {
          cardano_safe_memcpy(grown, new_capacity, scratch, total);
        }

        scratch  = grown;
        capacity = new_capacity;
      }

      if (chunk_len > 0U)
      {
        cardano_safe_memcpy(&scratch[total], capacity - total, &cursor->buf[cursor->offset], (size_t)chunk_len);
        total          += (size_t)chunk_len;
        cursor->offset += (size_t)chunk_len;
      }
    }

    return cardano_uplc_data_new_bytes(arena, scratch, total, out);
  }
}

/**
 * \brief Reads the element items of an array (definite or indefinite) into an arena
 *        pointer array.
 *
 * \param[in] arena The arena.
 * \param[in,out] cursor The byte cursor, positioned just past the array head byte.
 * \param[in] info The additional information from the head byte.
 * \param[in] depth The current recursion depth.
 * \param[out] out_items On success, the arena item array, or NULL when empty.
 * \param[out] out_count On success, the item count.
 *
 * \return \ref CARDANO_SUCCESS on success, or an error code.
 */
static cardano_error_t
parse_array(
  cardano_uplc_arena_t* arena,
  cursor_t*             cursor,
  byte_t                info,
  uint32_t              depth,
  // cppcheck-suppress misra-c2012-18.5; Reason: pointer nesting required by the API shape
  const cardano_uplc_data_t* const** out_items,
  size_t*                            out_count)
{
  const cardano_uplc_data_t** items      = NULL;
  size_t                      capacity   = 0U;
  size_t                      count      = 0U;
  bool                        indefinite = (info == (byte_t)PRV_CBOR_INFO_INDEFINITE);
  uint64_t                    length     = 0U;
  cardano_error_t             result;

  if (!indefinite)
  {
    result = read_argument(cursor, info, &length);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    if (length > (uint64_t)(cursor->size - cursor->offset))
    {
      return CARDANO_ERROR_DECODING;
    }

    if (length > 0U)
    {
      capacity = (size_t)length;
      items    = (const cardano_uplc_data_t**)cardano_uplc_arena_alloc(arena, sizeof(*items) * capacity, 0U);

      if (items == NULL)
      {
        return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
      }
    }
  }

  for (;;)
  // cppcheck-suppress misra-c2012-15.4; Reason: multiple loop exits keep the control flow flat
  {
    cardano_uplc_data_t* item = NULL;

    if (indefinite)
    {
      if (cursor->offset >= cursor->size)
      {
        return CARDANO_ERROR_DECODING;
      }

      if (cursor->buf[cursor->offset] == (byte_t)PRV_CBOR_BREAK)
      {
        ++cursor->offset;

        break;
      }
    }
    else if (count == (size_t)length)
    {
      break;
    }
    else
    {
      /* Definite array still has items to read. */
    }

    // cppcheck-suppress misra-c2012-17.2; Reason: bounded-depth recursion limited by program/data nesting and the execution budget
    result = parse_data_node(arena, cursor, depth + 1U, &item);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    if (count == capacity)
    {
      size_t                      new_capacity = (capacity == 0U) ? 4U : (capacity * 2U);
      const cardano_uplc_data_t** grown        = (const cardano_uplc_data_t**)cardano_uplc_arena_alloc(arena, sizeof(*grown) * new_capacity, 0U);

      if (grown == NULL)
      {
        return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
      }

      if (count > 0U)
      {
        cardano_safe_memcpy(grown, sizeof(*grown) * new_capacity, items, sizeof(*grown) * count);
      }

      items    = grown;
      capacity = new_capacity;
    }

    items[count] = item;
    ++count;
  }

  *out_items = (const cardano_uplc_data_t* const*)items;
  *out_count = count;

  return CARDANO_SUCCESS;
}

/**
 * \brief Reads the (key, value) pairs of a map (definite or indefinite) into an arena
 *        entry array.
 *
 * \param[in] arena The arena.
 * \param[in,out] cursor The byte cursor, positioned just past the map head byte.
 * \param[in] info The additional information from the head byte.
 * \param[in] depth The current recursion depth.
 * \param[out] out_entries On success, the arena entry array, or NULL when empty.
 * \param[out] out_count On success, the entry count.
 * \param[out] out_indefinite On success, whether the map was indefinite-length.
 *
 * \return \ref CARDANO_SUCCESS on success, or an error code.
 */
static cardano_error_t
parse_map(
  cardano_uplc_arena_t*            arena,
  cursor_t*                        cursor,
  byte_t                           info,
  uint32_t                         depth,
  const cardano_uplc_data_pair_t** out_entries,
  size_t*                          out_count,
  bool*                            out_indefinite)
{
  cardano_uplc_data_pair_t* entries    = NULL;
  size_t                    capacity   = 0U;
  size_t                    count      = 0U;
  bool                      indefinite = (info == (byte_t)PRV_CBOR_INFO_INDEFINITE);
  uint64_t                  length     = 0U;
  cardano_error_t           result;

  *out_indefinite = indefinite;

  if (!indefinite)
  {
    result = read_argument(cursor, info, &length);

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    if (length > ((uint64_t)(cursor->size - cursor->offset)))
    {
      return CARDANO_ERROR_DECODING;
    }

    if (length > 0U)
    {
      capacity = (size_t)length;
      entries  = (cardano_uplc_data_pair_t*)cardano_uplc_arena_alloc(arena, sizeof(*entries) * capacity, 0U);

      if (entries == NULL)
      {
        return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
      }
    }
  }

  for (;;)
  // cppcheck-suppress misra-c2012-15.4; Reason: multiple loop exits keep the control flow flat
  {
    cardano_uplc_data_t* key   = NULL;
    cardano_uplc_data_t* value = NULL;

    if (indefinite)
    {
      if (cursor->offset >= cursor->size)
      {
        return CARDANO_ERROR_DECODING;
      }

      if (cursor->buf[cursor->offset] == (byte_t)PRV_CBOR_BREAK)
      {
        ++cursor->offset;

        break;
      }
    }
    else if (count == (size_t)length)
    {
      break;
    }
    else
    {
      /* Definite map still has pairs to read. */
    }

    // cppcheck-suppress misra-c2012-17.2; Reason: bounded-depth recursion limited by program/data nesting and the execution budget
    result = parse_data_node(arena, cursor, depth + 1U, &key);

    if (result == CARDANO_SUCCESS)
    {
      // cppcheck-suppress misra-c2012-17.2; Reason: bounded-depth recursion limited by program/data nesting and the execution budget
      result = parse_data_node(arena, cursor, depth + 1U, &value);
    }

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    if (count == capacity)
    {
      size_t                    new_capacity = (capacity == 0U) ? 4U : (capacity * 2U);
      cardano_uplc_data_pair_t* grown        = (cardano_uplc_data_pair_t*)cardano_uplc_arena_alloc(arena, sizeof(*grown) * new_capacity, 0U);

      if (grown == NULL)
      {
        return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
      }

      if (count > 0U)
      {
        cardano_safe_memcpy(grown, sizeof(*grown) * new_capacity, entries, sizeof(*grown) * count);
      }

      entries  = grown;
      capacity = new_capacity;
    }

    entries[count].key   = key;
    entries[count].value = value;
    ++count;
  }

  *out_entries = entries;
  *out_count   = count;

  return CARDANO_SUCCESS;
}

/**
 * \brief Reads a bignum (tag 2 or 3) magnitude byte string and builds the integer
 *        node.
 *
 * Replicates \c _cardano_reader_read_bigint exactly: the magnitude is the big-endian
 * unsigned value of the byte string, and tag 3 negates it (yielding \c -magnitude,
 * matching the library's \c cardano_bigint_negate). The node down-converts to an
 * inline \c int64_t when it fits.
 *
 * \param[in] arena The arena.
 * \param[in,out] cursor The byte cursor, positioned just past the tag head byte.
 * \param[in] negative Whether the tag was the negative-bignum tag 3.
 * \param[out] out On success, the integer node.
 *
 * \return \ref CARDANO_SUCCESS on success, or an error code.
 */
static cardano_error_t
parse_bignum(cardano_uplc_arena_t* arena, cursor_t* cursor, bool negative, cardano_uplc_data_t** out)
{
  cardano_uplc_data_t* bytes_node = NULL;
  byte_t               major      = 0U;
  byte_t               info       = 0U;
  cardano_bigint_t*    magnitude  = NULL;
  cardano_error_t      result     = read_head(cursor, &major, &info);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  if (major != (byte_t)PRV_CBOR_MAJOR_BYTES)
  {
    return CARDANO_ERROR_DECODING;
  }

  result = parse_bytes(arena, cursor, info, &bytes_node);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = cardano_bigint_from_bytes(
    (bytes_node->as.bytes.data != NULL) ? bytes_node->as.bytes.data : (const byte_t*)"",
    bytes_node->as.bytes.size,
    CARDANO_BYTE_ORDER_BIG_ENDIAN,
    &magnitude);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  if (negative)
  {
    cardano_bigint_t* negated = NULL;

    result = cardano_bigint_clone(magnitude, &negated);

    if (result != CARDANO_SUCCESS)
    {
      cardano_bigint_unref(&magnitude);

      return result;
    }

    cardano_bigint_negate(magnitude, negated);
    cardano_bigint_unref(&magnitude);

    return node_from_bigint(arena, negated, out);
  }

  return node_from_bigint(arena, magnitude, out);
}

/**
 * \brief Reads a tagged item: a bignum, or a constructor in one of its three forms.
 *
 * \param[in] arena The arena.
 * \param[in,out] cursor The byte cursor, positioned just past the tag head byte.
 * \param[in] info The additional information from the tag head byte.
 * \param[in] depth The current recursion depth.
 * \param[out] out On success, the parsed node.
 *
 * \return \ref CARDANO_SUCCESS on success, or an error code.
 */
static cardano_error_t
parse_tagged(cardano_uplc_arena_t* arena, cursor_t* cursor, byte_t info, uint32_t depth, cardano_uplc_data_t** out)
{
  uint64_t        tag    = 0U;
  cardano_error_t result = read_argument(cursor, info, &tag);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  if (tag == (uint64_t)CARDANO_CBOR_TAG_UNSIGNED_BIG_NUM)
  {
    return parse_bignum(arena, cursor, false, out);
  }

  if (tag == (uint64_t)CARDANO_CBOR_TAG_NEGATIVE_BIG_NUM)
  {
    return parse_bignum(arena, cursor, true, out);
  }

  {
    uint64_t                          alternative = 0U;
    const cardano_uplc_data_t* const* fields      = NULL;
    size_t                            count       = 0U;
    byte_t                            major       = 0U;
    byte_t                            inner_info  = 0U;

    if (tag == PRV_CONSTR_GENERAL_FORM_TAG)
    {
      uint64_t array_len = 0U;
      byte_t   alt_major = 0U;
      byte_t   alt_info  = 0U;

      result = read_head(cursor, &major, &inner_info);

      if ((result == CARDANO_SUCCESS) && (major != (byte_t)PRV_CBOR_MAJOR_ARRAY))
      {
        return CARDANO_ERROR_DECODING;
      }

      if (result == CARDANO_SUCCESS)
      {
        result = read_argument(cursor, inner_info, &array_len);
      }

      if ((result == CARDANO_SUCCESS) && (array_len != 2U))
      {
        return CARDANO_ERROR_DECODING;
      }

      if (result == CARDANO_SUCCESS)
      {
        result = read_head(cursor, &alt_major, &alt_info);
      }

      if ((result == CARDANO_SUCCESS) && (alt_major != (byte_t)PRV_CBOR_MAJOR_UNSIGNED))
      {
        return CARDANO_ERROR_DECODING;
      }

      if (result == CARDANO_SUCCESS)
      {
        result = read_argument(cursor, alt_info, &alternative);
      }

      if (result == CARDANO_SUCCESS)
      {
        result = read_head(cursor, &major, &inner_info);
      }

      if ((result == CARDANO_SUCCESS) && (major != (byte_t)PRV_CBOR_MAJOR_ARRAY))
      {
        return CARDANO_ERROR_DECODING;
      }

      if (result == CARDANO_SUCCESS)
      {
        // cppcheck-suppress misra-c2012-17.2; Reason: bounded-depth recursion limited by program/data nesting and the execution budget
        result = parse_array(arena, cursor, inner_info, depth, &fields, &count);
      }
    }
    else if ((tag >= PRV_CONSTR_COMPACT_TAG_LO) && (tag <= PRV_CONSTR_COMPACT_TAG_HI))
    {
      alternative = tag - PRV_CONSTR_COMPACT_TAG_LO;

      result = read_head(cursor, &major, &inner_info);

      if ((result == CARDANO_SUCCESS) && (major != (byte_t)PRV_CBOR_MAJOR_ARRAY))
      {
        return CARDANO_ERROR_DECODING;
      }

      if (result == CARDANO_SUCCESS)
      {
        // cppcheck-suppress misra-c2012-17.2; Reason: bounded-depth recursion limited by program/data nesting and the execution budget
        result = parse_array(arena, cursor, inner_info, depth, &fields, &count);
      }
    }
    else if ((tag >= PRV_CONSTR_RANGED_TAG_LO) && (tag <= PRV_CONSTR_RANGED_TAG_HI))
    {
      alternative = (tag - PRV_CONSTR_RANGED_TAG_LO) + PRV_CONSTR_RANGED_OFFSET;

      result = read_head(cursor, &major, &inner_info);

      if ((result == CARDANO_SUCCESS) && (major != (byte_t)PRV_CBOR_MAJOR_ARRAY))
      {
        return CARDANO_ERROR_DECODING;
      }

      if (result == CARDANO_SUCCESS)
      {
        // cppcheck-suppress misra-c2012-17.2; Reason: bounded-depth recursion limited by program/data nesting and the execution budget
        result = parse_array(arena, cursor, inner_info, depth, &fields, &count);
      }
    }
    else
    {
      return CARDANO_ERROR_DECODING;
    }

    if (result != CARDANO_SUCCESS)
    {
      return result;
    }

    return cardano_uplc_data_new_constr(arena, alternative, fields, count, out);
  }
}

static cardano_error_t
parse_data_node(cardano_uplc_arena_t* arena, cursor_t* cursor, uint32_t depth, cardano_uplc_data_t** out)
{
  byte_t          major  = 0U;
  byte_t          info   = 0U;
  cardano_error_t result = CARDANO_SUCCESS;

  if (depth >= CARDANO_UPLC_DATA_MAX_DEPTH)
  {
    return CARDANO_ERROR_DECODING;
  }

  result = read_head(cursor, &major, &info);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  switch ((cbor_major_t)major)
  {
    case PRV_CBOR_MAJOR_UNSIGNED:
    {
      uint64_t value = 0U;

      result = read_argument(cursor, info, &value);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      if (value <= (uint64_t)INT64_MAX)
      {
        return cardano_uplc_data_new_integer_small(arena, (int64_t)value, out);
      }

      {
        cardano_bigint_t* integer = NULL;

        result = cardano_bigint_from_unsigned_int(value, &integer);

        if (result != CARDANO_SUCCESS)
        {
          return result;
        }

        return node_from_bigint(arena, integer, out);
      }
    }
    case PRV_CBOR_MAJOR_NEGATIVE:
    {
      uint64_t value = 0U;

      result = read_argument(cursor, info, &value);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      {
        uint64_t bits = (uint64_t)(-1) - value;
        int64_t  decoded;

        cardano_safe_memcpy(&decoded, sizeof(decoded), &bits, sizeof(decoded));

        return cardano_uplc_data_new_integer_small(arena, decoded, out);
      }
    }
    case PRV_CBOR_MAJOR_BYTES:
    {
      return parse_bytes(arena, cursor, info, out);
    }
    case PRV_CBOR_MAJOR_ARRAY:
    {
      const cardano_uplc_data_t* const* items = NULL;
      size_t                            count = 0U;

      // cppcheck-suppress misra-c2012-17.2; Reason: bounded-depth recursion limited by program/data nesting and the execution budget
      result = parse_array(arena, cursor, info, depth, &items, &count);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      return cardano_uplc_data_new_list(arena, items, count, out);
    }
    case PRV_CBOR_MAJOR_MAP:
    {
      const cardano_uplc_data_pair_t* entries    = NULL;
      size_t                          count      = 0U;
      bool                            indefinite = false;

      // cppcheck-suppress misra-c2012-17.2; Reason: bounded-depth recursion limited by program/data nesting and the execution budget
      result = parse_map(arena, cursor, info, depth, &entries, &count, &indefinite);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      return cardano_uplc_data_new_map(arena, entries, count, indefinite, out);
    }
    case PRV_CBOR_MAJOR_TAG:
    {
      // cppcheck-suppress misra-c2012-17.2; Reason: bounded-depth recursion limited by program/data nesting and the execution budget
      return parse_tagged(arena, cursor, info, depth, out);
    }
    default:
    {
      return CARDANO_ERROR_DECODING;
    }
  }
}

/**
 * \brief Recursively converts a library plutus-data node into an arena node.
 *
 * \param[in] arena The arena.
 * \param[in] data The library node.
 * \param[in] depth The current recursion depth.
 * \param[out] out On success, the arena node.
 *
 * \return \ref CARDANO_SUCCESS on success, or an error code.
 */
static cardano_error_t
from_plutus_data(cardano_uplc_arena_t* arena, const cardano_plutus_data_t* data, uint32_t depth, cardano_uplc_data_t** out)
{
  cardano_plutus_data_kind_t kind   = CARDANO_PLUTUS_DATA_KIND_INTEGER;
  cardano_error_t            result = CARDANO_SUCCESS;

  if (depth >= CARDANO_UPLC_DATA_MAX_DEPTH)
  {
    return CARDANO_ERROR_DECODING;
  }

  result = cardano_plutus_data_get_kind(data, &kind);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  switch (kind)
  {
    case CARDANO_PLUTUS_DATA_KIND_CONSTR:
    {
      cardano_constr_plutus_data_t* constr      = NULL;
      cardano_plutus_list_t*        fields      = NULL;
      uint64_t                      alternative = 0U;
      size_t                        count       = 0U;
      const cardano_uplc_data_t**   nodes       = NULL;
      size_t                        i           = 0U;

      // cppcheck-suppress misra-c2012-11.8; Reason: interfacing a non-const-correct API
      result = cardano_plutus_data_to_constr((cardano_plutus_data_t*)((const void*)data), &constr);

      if (result == CARDANO_SUCCESS)
      {
        result = cardano_constr_plutus_data_get_alternative(constr, &alternative);
      }

      if (result == CARDANO_SUCCESS)
      {
        result = cardano_constr_plutus_data_get_data(constr, &fields);
      }

      cardano_constr_plutus_data_unref(&constr);

      if (result != CARDANO_SUCCESS)
      {
        cardano_plutus_list_unref(&fields);

        return result;
      }

      count = cardano_plutus_list_get_length(fields);

      if (count > 0U)
      {
        nodes = (const cardano_uplc_data_t**)cardano_uplc_arena_alloc(arena, sizeof(*nodes) * count, 0U);

        if (nodes == NULL)
        {
          cardano_plutus_list_unref(&fields);

          return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
        }
      }

      for (i = 0U; i < count; ++i)
      {
        cardano_plutus_data_t* element = NULL;
        cardano_uplc_data_t*   node    = NULL;

        result = cardano_plutus_list_get(fields, i, &element);

        if (result == CARDANO_SUCCESS)
        {
          // cppcheck-suppress misra-c2012-17.2; Reason: bounded-depth recursion limited by program/data nesting and the execution budget
          result = from_plutus_data(arena, element, depth + 1U, &node);
        }

        cardano_plutus_data_unref(&element);

        if (result != CARDANO_SUCCESS)
        {
          cardano_plutus_list_unref(&fields);

          return result;
        }

        nodes[i] = node;
      }

      cardano_plutus_list_unref(&fields);

      return cardano_uplc_data_new_constr(arena, alternative, (const cardano_uplc_data_t* const*)nodes, count, out);
    }
    case CARDANO_PLUTUS_DATA_KIND_MAP:
    {
      cardano_plutus_map_t*     map     = NULL;
      cardano_plutus_list_t*    keys    = NULL;
      cardano_plutus_list_t*    values  = NULL;
      cardano_uplc_data_pair_t* entries = NULL;
      size_t                    count   = 0U;
      size_t                    i       = 0U;

      // cppcheck-suppress misra-c2012-11.8; Reason: interfacing a non-const-correct API
      result = cardano_plutus_data_to_map((cardano_plutus_data_t*)((const void*)data), &map);

      if (result == CARDANO_SUCCESS)
      {
        result = cardano_plutus_map_get_keys(map, &keys);
      }

      if (result == CARDANO_SUCCESS)
      {
        result = cardano_plutus_map_get_values(map, &values);
      }

      if (result != CARDANO_SUCCESS)
      {
        cardano_plutus_list_unref(&keys);
        cardano_plutus_list_unref(&values);
        cardano_plutus_map_unref(&map);

        return result;
      }

      count = cardano_plutus_map_get_length(map);

      if (count > 0U)
      {
        entries = (cardano_uplc_data_pair_t*)cardano_uplc_arena_alloc(arena, sizeof(*entries) * count, 0U);

        if (entries == NULL)
        {
          cardano_plutus_list_unref(&keys);
          cardano_plutus_list_unref(&values);
          cardano_plutus_map_unref(&map);

          return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
        }
      }

      for (i = 0U; i < count; ++i)
      {
        cardano_plutus_data_t* key      = NULL;
        cardano_plutus_data_t* value    = NULL;
        cardano_uplc_data_t*   key_node = NULL;
        cardano_uplc_data_t*   val_node = NULL;

        result = cardano_plutus_list_get(keys, i, &key);

        if (result == CARDANO_SUCCESS)
        {
          result = cardano_plutus_list_get(values, i, &value);
        }

        if (result == CARDANO_SUCCESS)
        {
          // cppcheck-suppress misra-c2012-17.2; Reason: bounded-depth recursion limited by program/data nesting and the execution budget
          result = from_plutus_data(arena, key, depth + 1U, &key_node);
        }

        if (result == CARDANO_SUCCESS)
        {
          // cppcheck-suppress misra-c2012-17.2; Reason: bounded-depth recursion limited by program/data nesting and the execution budget
          result = from_plutus_data(arena, value, depth + 1U, &val_node);
        }

        cardano_plutus_data_unref(&key);
        cardano_plutus_data_unref(&value);

        if (result != CARDANO_SUCCESS)
        {
          cardano_plutus_list_unref(&keys);
          cardano_plutus_list_unref(&values);
          cardano_plutus_map_unref(&map);

          return result;
        }

        entries[i].key   = key_node;
        entries[i].value = val_node;
      }

      cardano_plutus_list_unref(&keys);
      cardano_plutus_list_unref(&values);
      cardano_plutus_map_unref(&map);

      return cardano_uplc_data_new_map(arena, entries, count, false, out);
    }
    case CARDANO_PLUTUS_DATA_KIND_LIST:
    {
      cardano_plutus_list_t*      list  = NULL;
      const cardano_uplc_data_t** nodes = NULL;
      size_t                      count = 0U;
      size_t                      i     = 0U;

      // cppcheck-suppress misra-c2012-11.8; Reason: interfacing a non-const-correct API
      result = cardano_plutus_data_to_list((cardano_plutus_data_t*)((const void*)data), &list);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      count = cardano_plutus_list_get_length(list);

      if (count > 0U)
      {
        nodes = (const cardano_uplc_data_t**)cardano_uplc_arena_alloc(arena, sizeof(*nodes) * count, 0U);

        if (nodes == NULL)
        {
          cardano_plutus_list_unref(&list);

          return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
        }
      }

      for (i = 0U; i < count; ++i)
      {
        cardano_plutus_data_t* element = NULL;
        cardano_uplc_data_t*   node    = NULL;

        result = cardano_plutus_list_get(list, i, &element);

        if (result == CARDANO_SUCCESS)
        {
          // cppcheck-suppress misra-c2012-17.2; Reason: bounded-depth recursion limited by program/data nesting and the execution budget
          result = from_plutus_data(arena, element, depth + 1U, &node);
        }

        cardano_plutus_data_unref(&element);

        if (result != CARDANO_SUCCESS)
        {
          cardano_plutus_list_unref(&list);

          return result;
        }

        nodes[i] = node;
      }

      cardano_plutus_list_unref(&list);

      return cardano_uplc_data_new_list(arena, (const cardano_uplc_data_t* const*)nodes, count, out);
    }
    case CARDANO_PLUTUS_DATA_KIND_INTEGER:
    {
      cardano_bigint_t* integer = NULL;

      result = cardano_plutus_data_to_integer(data, &integer);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      result = cardano_uplc_data_new_integer(arena, integer, out);

      cardano_bigint_unref(&integer);

      return result;
    }
    case CARDANO_PLUTUS_DATA_KIND_BYTES:
    {
      cardano_buffer_t* bytes = NULL;

      // cppcheck-suppress misra-c2012-11.8; Reason: interfacing a non-const-correct API
      result = cardano_plutus_data_to_bounded_bytes((cardano_plutus_data_t*)((const void*)data), &bytes);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      result = cardano_uplc_data_new_bytes(arena, cardano_buffer_get_data(bytes), cardano_buffer_get_size(bytes), out);

      cardano_buffer_unref(&bytes);

      return result;
    }
    default:
    {
      return CARDANO_ERROR_INVALID_ARGUMENT;
    }
  }
}

/**
 * \brief Recursively converts an arena data node into a library plutus-data node.
 *
 * \param[in] data The arena node.
 * \param[in] depth The current recursion depth.
 * \param[out] out On success, the owned library node.
 *
 * \return \ref CARDANO_SUCCESS on success, or an error code.
 */
static cardano_error_t
to_plutus_data(const cardano_uplc_data_t* data, uint32_t depth, cardano_plutus_data_t** out)
{
  cardano_error_t result = CARDANO_SUCCESS;
  size_t          i      = 0U;

  if (data == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (depth >= CARDANO_UPLC_DATA_MAX_DEPTH)
  {
    return CARDANO_ERROR_DECODING;
  }

  switch (data->kind)
  {
    case CARDANO_UPLC_DATA_KIND_CONSTR:
    {
      cardano_plutus_list_t*        fields = NULL;
      cardano_constr_plutus_data_t* constr = NULL;

      result = cardano_plutus_list_new(&fields);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      for (i = 0U; i < data->as.constr.count; ++i)
      {
        cardano_plutus_data_t* element = NULL;

        // cppcheck-suppress misra-c2012-17.2; Reason: bounded-depth recursion limited by program/data nesting and the execution budget
        result = to_plutus_data(data->as.constr.fields[i], depth + 1U, &element);

        if (result == CARDANO_SUCCESS)
        {
          result = cardano_plutus_list_add(fields, element);
        }

        cardano_plutus_data_unref(&element);

        if (result != CARDANO_SUCCESS)
        {
          cardano_plutus_list_unref(&fields);

          return result;
        }
      }

      result = cardano_constr_plutus_data_new(data->as.constr.tag, fields, &constr);

      cardano_plutus_list_unref(&fields);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      result = cardano_plutus_data_new_constr(constr, out);

      cardano_constr_plutus_data_unref(&constr);

      return result;
    }
    case CARDANO_UPLC_DATA_KIND_MAP:
    {
      cardano_plutus_map_t* map = NULL;

      result = cardano_plutus_map_new(&map);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      for (i = 0U; i < data->as.map.count; ++i)
      {
        cardano_plutus_data_t* key   = NULL;
        cardano_plutus_data_t* value = NULL;

        // cppcheck-suppress misra-c2012-17.2; Reason: bounded-depth recursion limited by program/data nesting and the execution budget
        result = to_plutus_data(data->as.map.entries[i].key, depth + 1U, &key);

        if (result == CARDANO_SUCCESS)
        {
          // cppcheck-suppress misra-c2012-17.2; Reason: bounded-depth recursion limited by program/data nesting and the execution budget
          result = to_plutus_data(data->as.map.entries[i].value, depth + 1U, &value);
        }

        if (result == CARDANO_SUCCESS)
        {
          result = cardano_plutus_map_insert(map, key, value);
        }

        cardano_plutus_data_unref(&key);
        cardano_plutus_data_unref(&value);

        if (result != CARDANO_SUCCESS)
        {
          cardano_plutus_map_unref(&map);

          return result;
        }
      }

      result = cardano_plutus_data_new_map(map, out);

      cardano_plutus_map_unref(&map);

      return result;
    }
    case CARDANO_UPLC_DATA_KIND_LIST:
    {
      cardano_plutus_list_t* list = NULL;

      result = cardano_plutus_list_new(&list);

      if (result != CARDANO_SUCCESS)
      {
        return result;
      }

      for (i = 0U; i < data->as.list.count; ++i)
      {
        cardano_plutus_data_t* element = NULL;

        // cppcheck-suppress misra-c2012-17.2; Reason: bounded-depth recursion limited by program/data nesting and the execution budget
        result = to_plutus_data(data->as.list.items[i], depth + 1U, &element);

        if (result == CARDANO_SUCCESS)
        {
          result = cardano_plutus_list_add(list, element);
        }

        cardano_plutus_data_unref(&element);

        if (result != CARDANO_SUCCESS)
        {
          cardano_plutus_list_unref(&list);

          return result;
        }
      }

      result = cardano_plutus_data_new_list(list, out);

      cardano_plutus_list_unref(&list);

      return result;
    }
    case CARDANO_UPLC_DATA_KIND_INTEGER:
    {
      if (data->as.integer.is_small)
      {
        cardano_bigint_t* integer = NULL;

        result = cardano_bigint_from_int(data->as.integer.small, &integer);

        if (result != CARDANO_SUCCESS)
        {
          return result;
        }

        result = cardano_plutus_data_new_integer(integer, out);

        cardano_bigint_unref(&integer);

        return result;
      }

      return cardano_plutus_data_new_integer(data->as.integer.big, out);
    }
    case CARDANO_UPLC_DATA_KIND_BYTES:
    {
      static const byte_t empty = 0U;
      const byte_t*       bytes = (data->as.bytes.data != NULL) ? data->as.bytes.data : &empty;

      return cardano_plutus_data_new_bytes(bytes, data->as.bytes.size, out);
    }
    default:
    {
      return CARDANO_ERROR_INVALID_ARGUMENT;
    }
  }
}

/* DEFINITIONS ***************************************************************/

cardano_error_t
cardano_uplc_data_new_constr(
  cardano_uplc_arena_t*             arena,
  uint64_t                          tag,
  const cardano_uplc_data_t* const* fields,
  size_t                            count,
  cardano_uplc_data_t**             out)
{
  cardano_uplc_data_t* node = NULL;

  if ((arena == NULL) || (out == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if ((fields == NULL) && (count > 0U))
  {
    return CARDANO_ERROR_INVALID_ARGUMENT;
  }

  node = alloc_node(arena);

  if (node == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  node->kind             = CARDANO_UPLC_DATA_KIND_CONSTR;
  node->as.constr.tag    = tag;
  node->as.constr.fields = fields;
  node->as.constr.count  = count;

  *out = node;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_uplc_data_new_map(
  cardano_uplc_arena_t*           arena,
  const cardano_uplc_data_pair_t* entries,
  size_t                          count,
  bool                            indefinite,
  cardano_uplc_data_t**           out)
{
  cardano_uplc_data_t* node = NULL;

  if ((arena == NULL) || (out == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if ((entries == NULL) && (count > 0U))
  {
    return CARDANO_ERROR_INVALID_ARGUMENT;
  }

  node = alloc_node(arena);

  if (node == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  node->kind              = CARDANO_UPLC_DATA_KIND_MAP;
  node->as.map.entries    = entries;
  node->as.map.count      = count;
  node->as.map.indefinite = indefinite;

  *out = node;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_uplc_data_new_list(
  cardano_uplc_arena_t*             arena,
  const cardano_uplc_data_t* const* items,
  size_t                            count,
  cardano_uplc_data_t**             out)
{
  cardano_uplc_data_t* node = NULL;

  if ((arena == NULL) || (out == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if ((items == NULL) && (count > 0U))
  {
    return CARDANO_ERROR_INVALID_ARGUMENT;
  }

  node = alloc_node(arena);

  if (node == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  node->kind          = CARDANO_UPLC_DATA_KIND_LIST;
  node->as.list.items = items;
  node->as.list.count = count;

  *out = node;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_uplc_data_new_integer_small(
  cardano_uplc_arena_t* arena,
  int64_t               value,
  cardano_uplc_data_t** out)
{
  cardano_uplc_data_t* node = NULL;

  if ((arena == NULL) || (out == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  node = alloc_node(arena);

  if (node == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  node->kind                = CARDANO_UPLC_DATA_KIND_INTEGER;
  node->as.integer.is_small = true;
  node->as.integer.small    = value;
  node->as.integer.big      = NULL;

  *out = node;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_uplc_data_new_integer(
  cardano_uplc_arena_t* arena,
  cardano_bigint_t*     value,
  cardano_uplc_data_t** out)
{
  cardano_uplc_data_t* node    = NULL;
  int64_t              inline_ = 0;

  if ((arena == NULL) || (value == NULL) || (out == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if (cardano_uplc_int_bigint_fits_int64(value, &inline_))
  {
    return cardano_uplc_data_new_integer_small(arena, inline_, out);
  }

  node = alloc_node(arena);

  if (node == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  cardano_bigint_ref(value);

  if (cardano_uplc_arena_register_unref(arena, value, &unref_bigint) != CARDANO_SUCCESS)
  {
    cardano_bigint_unref(&value);

    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  node->kind                = CARDANO_UPLC_DATA_KIND_INTEGER;
  node->as.integer.is_small = false;
  node->as.integer.small    = 0;
  node->as.integer.big      = value;

  *out = node;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_uplc_data_new_bytes(
  cardano_uplc_arena_t* arena,
  const byte_t*         bytes,
  size_t                size,
  cardano_uplc_data_t** out)
{
  cardano_uplc_data_t* node = NULL;
  byte_t*              copy = NULL;

  if ((arena == NULL) || (out == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if ((bytes == NULL) && (size > 0U))
  {
    return CARDANO_ERROR_INVALID_ARGUMENT;
  }

  node = alloc_node(arena);

  if (node == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  if (size > 0U)
  {
    copy = (byte_t*)cardano_uplc_arena_alloc(arena, size, 1U);

    if (copy == NULL)
    {
      return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
    }

    cardano_safe_memcpy(copy, size, bytes, size);
  }

  node->kind          = CARDANO_UPLC_DATA_KIND_BYTES;
  node->as.bytes.data = copy;
  node->as.bytes.size = size;

  *out = node;

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_uplc_data_integer_materialize(
  cardano_uplc_arena_t*      arena,
  const cardano_uplc_data_t* data,
  const cardano_bigint_t**   out)
{
  // cppcheck-suppress misra-c2012-11.8; Reason: interfacing a non-const-correct API
  cardano_uplc_data_t* mutable_node = (cardano_uplc_data_t*)((const void*)data);
  cardano_bigint_t*    built        = NULL;
  cardano_error_t      result       = CARDANO_SUCCESS;

  if (data->as.integer.big != NULL)
  {
    *out = data->as.integer.big;

    return CARDANO_SUCCESS;
  }

  result = cardano_bigint_from_int(data->as.integer.small, &built);

  if (result != CARDANO_SUCCESS)
  {
    return result;
  }

  result = cardano_uplc_arena_register_unref(arena, built, &unref_bigint);

  if (result != CARDANO_SUCCESS)
  {
    cardano_bigint_unref(&built);

    return result;
  }

  mutable_node->as.integer.big = built;
  *out                         = built;

  return CARDANO_SUCCESS;
}

bool
cardano_uplc_data_equals(const cardano_uplc_data_t* lhs, const cardano_uplc_data_t* rhs)
{
  equals_pair_t* stack    = NULL;
  size_t         capacity = 0U;
  size_t         count    = 0U;
  bool           equal    = true;

  if (!equals_push(&stack, &capacity, &count, lhs, rhs))
  {
    return false;
  }

  while (equal && (count > 0U))
  {
    // cppcheck-suppress misra-c2012-13.3; Reason: local post-increment with no aliasing
    equals_pair_t pair = stack[--count];

    const cardano_uplc_data_t* a = pair.lhs;
    const cardano_uplc_data_t* b = pair.rhs;

    size_t i = 0U;

    if (a == b)
    {
      continue;
    }

    if ((a == NULL) || (b == NULL) || (a->kind != b->kind))
    {
      equal = false;

      break;
    }

    switch (a->kind)
    {
      case CARDANO_UPLC_DATA_KIND_CONSTR:
      {
        if ((a->as.constr.tag != b->as.constr.tag) || (a->as.constr.count != b->as.constr.count))
        {
          equal = false;

          break;
        }

        for (i = 0U; equal && (i < a->as.constr.count); ++i)
        {
          if (!equals_push(&stack, &capacity, &count, a->as.constr.fields[i], b->as.constr.fields[i]))
          {
            equal = false;
          }
        }

        break;
      }
      case CARDANO_UPLC_DATA_KIND_MAP:
      {
        if (a->as.map.count != b->as.map.count)
        {
          equal = false;

          break;
        }

        for (i = 0U; equal && (i < a->as.map.count); ++i)
        {
          if (!equals_push(&stack, &capacity, &count, a->as.map.entries[i].key, b->as.map.entries[i].key) || !equals_push(&stack, &capacity, &count, a->as.map.entries[i].value, b->as.map.entries[i].value))
          {
            equal = false;
          }
        }

        break;
      }
      case CARDANO_UPLC_DATA_KIND_LIST:
      {
        if (a->as.list.count != b->as.list.count)
        {
          equal = false;

          break;
        }

        for (i = 0U; equal && (i < a->as.list.count); ++i)
        {
          if (!equals_push(&stack, &capacity, &count, a->as.list.items[i], b->as.list.items[i]))
          {
            equal = false;
          }
        }

        break;
      }
      case CARDANO_UPLC_DATA_KIND_INTEGER:
      {
        equal = integer_equals(a, b);

        break;
      }
      case CARDANO_UPLC_DATA_KIND_BYTES:
      {
        if (a->as.bytes.size != b->as.bytes.size)
        {
          equal = false;
        }
        else if (a->as.bytes.size > 0U)
        {
          equal = memcmp(a->as.bytes.data, b->as.bytes.data, a->as.bytes.size) == 0;
        }
        else
        {
          /* Equal empty byte strings: nothing to compare. */
        }

        break;
      }
      default:
      {
        equal = false;

        break;
      }
    }
  }

  _cardano_free(stack);

  return equal;
}

int64_t
cardano_uplc_data_node_ex_mem(const cardano_uplc_data_t* data)
{
  return compute_ex_mem(data);
}

int64_t
cardano_uplc_data_node_count(const cardano_uplc_data_t* data)
{
  return compute_node_count(data);
}

cardano_error_t
cardano_uplc_data_from_cbor_bytes(
  cardano_uplc_arena_t* arena,
  const byte_t*         bytes,
  size_t                size,
  cardano_uplc_data_t** out)
{
  cursor_t        cursor = { NULL, 0U, 0U };
  cardano_error_t result = CARDANO_SUCCESS;

  if ((arena == NULL) || (out == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  if ((bytes == NULL) && (size > 0U))
  {
    return CARDANO_ERROR_DECODING;
  }

  cursor.buf  = bytes;
  cursor.size = size;

  result = parse_data_node(arena, &cursor, 0U, out);

  if (result != CARDANO_SUCCESS)
  {
    return (result == CARDANO_ERROR_MEMORY_ALLOCATION_FAILED) ? result : CARDANO_ERROR_DECODING;
  }

  return CARDANO_SUCCESS;
}

cardano_error_t
cardano_uplc_data_to_cbor(const cardano_uplc_data_t* data, cardano_cbor_writer_t* writer)
{
  if ((data == NULL) || (writer == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  return to_cbor(data, writer);
}

cardano_error_t
cardano_uplc_data_from_plutus_data(
  cardano_uplc_arena_t*        arena,
  const cardano_plutus_data_t* data,
  cardano_uplc_data_t**        out)
{
  if ((arena == NULL) || (data == NULL) || (out == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  return from_plutus_data(arena, data, 0U, out);
}

cardano_error_t
cardano_uplc_data_to_plutus_data(
  const cardano_uplc_data_t* data,
  cardano_plutus_data_t**    out)
{
  if ((data == NULL) || (out == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  return to_plutus_data(data, 0U, out);
}
