/**
 * \file uplc_arena.c
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

/* INCLUDES ******************************************************************/

#include "uplc_arena.h"

#include "../../allocators.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* CONSTANTS *****************************************************************/

/**
 * \brief Default block payload size, used when the caller passes 0 to
 *        \ref cardano_uplc_arena_new.
 */
static const size_t ARENA_DEFAULT_BLOCK_SIZE = 64U * 1024U;

/**
 * \brief Upper bound on the total payload bytes the arena will serve.
 *
 * Decoding and evaluation of malformed input must not be able to allocate
 * without bound before the machine charges for it. The ceiling is the largest
 * Plutus execution-memory budget (about 14e6 ex-mem units) scaled by a generous
 * bytes-per-unit factor, giving a few hundred megabytes; large enough never to
 * bite a legitimate script, small enough to deny an unbounded allocation. Past
 * the ceiling, allocation fails and creation refuses with
 * \ref CARDANO_ERROR_ILLEGAL_STATE at the call sites that surface it.
 */
static const size_t ARENA_BYTE_CEILING = (size_t)512U * 1024U * 1024U;

/**
 * \brief Natural maximum alignment used when the caller passes align 0.
 */
static const size_t ARENA_MAX_ALIGN = sizeof(long double) > sizeof(void*) ? sizeof(long double) : sizeof(void*);

/* STRUCTURES ****************************************************************/

/**
 * \brief One backing block: a header followed by its payload.
 */
typedef struct cardano_uplc_arena_block_t
{
    struct cardano_uplc_arena_block_t* next;
    size_t                             capacity;
    size_t                             offset;
    byte_t*                            payload;
} cardano_uplc_arena_block_t;

/**
 * \brief One entry on the on-free unref list.
 */
typedef struct cardano_uplc_arena_unref_node_t
{
    struct cardano_uplc_arena_unref_node_t* next;
    void*                                   object;
    cardano_uplc_arena_unref_t              unref;
} cardano_uplc_arena_unref_node_t;

/**
 * \brief Region allocator state.
 */
struct cardano_uplc_arena_t
{
    cardano_uplc_arena_block_t*      blocks;
    cardano_uplc_arena_unref_node_t* unrefs;
    size_t                           block_size;
    size_t                           bytes_used;
    size_t                           byte_ceiling;
};

/* STATIC FUNCTIONS **********************************************************/

/**
 * \brief Tests whether a value is a non-zero power of two.
 *
 * \param[in] value The value to test.
 *
 * \return \c true if \p value is a power of two, \c false otherwise.
 */
static bool
is_power_of_two(const size_t value)
{
  return (value != 0U) && ((value & (value - 1U)) == 0U);
}

/**
 * \brief Computes the offset within a block at which an allocation of the given
 *        alignment must start so that its absolute address is aligned.
 *
 * Aligns the absolute address \p base + \p offset up to \p align and returns the
 * resulting distance from \p base, so \p base plus the returned offset is an
 * aligned pointer regardless of how the block payload itself is aligned.
 *
 * \param[in] base Address of the block payload.
 * \param[in] offset Bytes already consumed in the block.
 * \param[in] align The alignment; must be a power of two.
 * \param[out] result The aligned offset.
 *
 * \note Rounding up to a power-of-two alignment cannot overflow a live address:
 *       \p base + \p offset is the address of allocated memory and \p align - 1
 *       is at most \ref ARENA_MAX_ALIGN - 1, so their sum stays within the
 *       address space. No allocation can sit within \p align bytes of
 *       \c UINTPTR_MAX.
 */
static void
compute_aligned_offset(const byte_t* base, const size_t offset, const size_t align, size_t* result)
{
  const uintptr_t mask    = (uintptr_t)align - 1U;
  const uintptr_t address = (uintptr_t)base + (uintptr_t)offset;

  *result = (size_t)(((address + mask) & ~mask) - (uintptr_t)base);
}

/**
 * \brief Charges \p amount against the byte ceiling.
 *
 * \param[in] arena The arena whose running total is updated.
 * \param[in] amount The number of bytes to charge.
 *
 * \return \c true if the charge stays within the arena's ceiling and is applied,
 *         \c false if it would overflow or exceed that ceiling.
 */
static bool
charge_bytes(cardano_uplc_arena_t* arena, const size_t amount)
{
  if (amount > (arena->byte_ceiling - arena->bytes_used))
  {
    return false;
  }

  arena->bytes_used += amount;

  return true;
}

/**
 * \brief Allocates and links a new block with at least \p min_capacity payload
 *        bytes.
 *
 * \param[in] arena The arena to extend.
 * \param[in] min_capacity The minimum payload the block must hold.
 *
 * \return The new block on success, or \c NULL if the backing allocator fails.
 */
static cardano_uplc_arena_block_t*
grow_arena(cardano_uplc_arena_t* arena, const size_t min_capacity)
{
  size_t capacity = arena->block_size;

  if (capacity < min_capacity)
  {
    capacity = min_capacity;
  }

  if (capacity > (SIZE_MAX - sizeof(cardano_uplc_arena_block_t)))
  {
    return NULL;
  }

  cardano_uplc_arena_block_t* block = _cardano_malloc(sizeof(cardano_uplc_arena_block_t) + capacity);

  if (block == NULL)
  {
    return NULL;
  }

  block->capacity = capacity;
  block->offset   = 0U;
  block->payload  = (byte_t*)block + sizeof(cardano_uplc_arena_block_t);
  block->next     = arena->blocks;
  arena->blocks   = block;

  return block;
}

/* DEFINITIONS ***************************************************************/

cardano_error_t
cardano_uplc_arena_new(const size_t block_size, cardano_uplc_arena_t** arena)
{
  return cardano_uplc_int_arena_new_with_ceiling(block_size, ARENA_BYTE_CEILING, arena);
}

cardano_error_t
cardano_uplc_int_arena_new_with_ceiling(const size_t block_size, const size_t byte_ceiling, cardano_uplc_arena_t** arena)
{
  if (arena == NULL)
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_uplc_arena_t* result = _cardano_malloc(sizeof(cardano_uplc_arena_t));

  if (result == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  result->blocks       = NULL;
  result->unrefs       = NULL;
  result->bytes_used   = 0U;
  result->block_size   = (block_size == 0U) ? ARENA_DEFAULT_BLOCK_SIZE : block_size;
  result->byte_ceiling = byte_ceiling;

  *arena = result;

  return CARDANO_SUCCESS;
}

void*
cardano_uplc_arena_alloc(cardano_uplc_arena_t* arena, const size_t size, const size_t align)
{
  size_t                      effective_align = (align == 0U) ? ARENA_MAX_ALIGN : align;
  cardano_uplc_arena_block_t* block           = NULL;
  size_t                      aligned_offset  = 0U;
  size_t                      end             = 0U;
  size_t                      provisional     = 0U;

  if (arena == NULL)
  {
    return NULL;
  }

  if ((align != 0U) && !is_power_of_two(align))
  {
    return NULL;
  }

  block = arena->blocks;

  if (block != NULL)
  {
    const uintptr_t mask    = (uintptr_t)effective_align - 1U;
    const uintptr_t address = (uintptr_t)block->payload + (uintptr_t)block->offset;

    aligned_offset = (size_t)(((address + mask) & ~mask) - (uintptr_t)block->payload);

    if ((aligned_offset <= block->capacity) && (size <= (block->capacity - aligned_offset)))
    {
      size_t charge = (aligned_offset - block->offset) + size;

      if (charge <= (arena->byte_ceiling - arena->bytes_used))
      {
        arena->bytes_used += charge;
        block->offset     = aligned_offset + size;

        return block->payload + aligned_offset;
      }

      return NULL;
    }
  }

  if (size > (SIZE_MAX - (effective_align - 1U)))
  {
    return NULL;
  }

  provisional = size + (effective_align - 1U);

  if (!charge_bytes(arena, provisional))
  {
    return NULL;
  }

  block = grow_arena(arena, provisional);

  if (block == NULL)
  {
    arena->bytes_used -= provisional;

    return NULL;
  }

  compute_aligned_offset(block->payload, block->offset, effective_align, &aligned_offset);
  end = aligned_offset + size;

  arena->bytes_used -= (provisional - end);

  block->offset = end;

  return block->payload + aligned_offset;
}

cardano_error_t
cardano_uplc_arena_register_unref(cardano_uplc_arena_t* arena, void* object, cardano_uplc_arena_unref_t unref)
{
  if ((arena == NULL) || (object == NULL) || (unref == NULL))
  {
    return CARDANO_ERROR_POINTER_IS_NULL;
  }

  cardano_uplc_arena_unref_node_t* node = _cardano_malloc(sizeof(cardano_uplc_arena_unref_node_t));

  if (node == NULL)
  {
    return CARDANO_ERROR_MEMORY_ALLOCATION_FAILED;
  }

  if (!charge_bytes(arena, sizeof(cardano_uplc_arena_unref_node_t)))
  {
    _cardano_free(node);

    return CARDANO_ERROR_ILLEGAL_STATE;
  }

  node->object  = object;
  node->unref   = unref;
  node->next    = arena->unrefs;
  arena->unrefs = node;

  return CARDANO_SUCCESS;
}

size_t
cardano_uplc_arena_bytes_used(const cardano_uplc_arena_t* arena)
{
  if (arena == NULL)
  {
    return 0U;
  }

  return arena->bytes_used;
}

void
cardano_uplc_arena_free(cardano_uplc_arena_t** arena)
{
  if ((arena == NULL) || (*arena == NULL))
  {
    return;
  }

  cardano_uplc_arena_t*            self  = *arena;
  cardano_uplc_arena_unref_node_t* node  = self->unrefs;
  cardano_uplc_arena_block_t*      block = self->blocks;

  while (node != NULL)
  {
    cardano_uplc_arena_unref_node_t* next = node->next;

    node->unref(node->object);
    _cardano_free(node);

    node = next;
  }

  while (block != NULL)
  {
    cardano_uplc_arena_block_t* next = block->next;

    _cardano_free(block);

    block = next;
  }

  _cardano_free(self);

  *arena = NULL;
}

void
cardano_uplc_arena_reset(cardano_uplc_arena_t* arena)
{
  if (arena == NULL)
  {
    return;
  }

  cardano_uplc_arena_unref_node_t* node = arena->unrefs;

  while (node != NULL)
  {
    cardano_uplc_arena_unref_node_t* next = node->next;

    node->unref(node->object);
    _cardano_free(node);

    node = next;
  }

  arena->unrefs = NULL;

  cardano_uplc_arena_block_t* block = arena->blocks;

  while (block != NULL)
  {
    block->offset = 0U;
    block         = block->next;
  }

  arena->bytes_used = 0U;
}
