/**
 * \file arena.cpp
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

#include <cardano/common/bigint.h>
#include <cardano/error.h>
#include <cardano/object.h>

#include "../../src/uplc/arena/uplc_arena.h"

#include "../allocators_helpers.h"
#include "../src/allocators.h"

#include <cstdint>
#include <gmock/gmock.h>

/* STATIC HELPERS ************************************************************/

extern "C" {

static void
object_unref_adapter(void* object)
{
  cardano_object_t* obj = reinterpret_cast<cardano_object_t*>(object);
  cardano_object_unref(&obj);
}

static size_t g_counter_unref_calls = 0U;

static void
counting_unref(void* object)
{
  ++g_counter_unref_calls;
  *reinterpret_cast<size_t*>(object) += 1U;
}
}

static bool
is_aligned(const void* ptr, const size_t align)
{
  return (reinterpret_cast<uintptr_t>(ptr) % align) == 0U;
}

/* UNIT TESTS ****************************************************************/

TEST(cardano_uplc_arena_new, createsArena)
{
  // Arrange
  cardano_uplc_arena_t* arena = nullptr;

  // Act
  cardano_error_t error = cardano_uplc_arena_new(1024U, &arena);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);
  EXPECT_THAT(arena, testing::Not((cardano_uplc_arena_t*)nullptr));
  EXPECT_EQ(cardano_uplc_arena_bytes_used(arena), 0U);

  // Cleanup
  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_arena_new, usesDefaultBlockSizeWhenZero)
{
  // Arrange
  cardano_uplc_arena_t* arena = nullptr;

  // Act
  cardano_error_t error = cardano_uplc_arena_new(0U, &arena);

  // Assert
  EXPECT_EQ(error, CARDANO_SUCCESS);

  void* ptr = cardano_uplc_arena_alloc(arena, 4096U, 8U);
  EXPECT_THAT(ptr, testing::Not((void*)nullptr));

  // Cleanup
  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_arena_new, returnsErrorIfArenaIsNull)
{
  // Act
  cardano_error_t error = cardano_uplc_arena_new(1024U, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_uplc_arena_new, returnsErrorIfMemoryAllocationFails)
{
  // Arrange
  // Pre-set to a sentinel so the test asserts the contract that the output
  // pointer is left untouched on failure, not merely that it stays null.
  cardano_uplc_arena_t* const sentinel = reinterpret_cast<cardano_uplc_arena_t*>(0x1);
  cardano_uplc_arena_t*       arena    = sentinel;

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_uplc_arena_new(1024U, &arena);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(arena, sentinel);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
}

TEST(cardano_uplc_arena_alloc, returnsNullWhenArenaIsNull)
{
  // Act
  void* ptr = cardano_uplc_arena_alloc(nullptr, 8U, 8U);

  // Assert
  EXPECT_EQ(ptr, (void*)nullptr);
}

TEST(cardano_uplc_arena_alloc, returnsNullWhenAlignmentIsNotPowerOfTwo)
{
  // Arrange
  cardano_uplc_arena_t* arena = nullptr;
  ASSERT_EQ(cardano_uplc_arena_new(1024U, &arena), CARDANO_SUCCESS);

  // Act
  void* ptr = cardano_uplc_arena_alloc(arena, 8U, 3U);

  // Assert
  EXPECT_EQ(ptr, (void*)nullptr);

  // Cleanup
  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_arena_alloc, honorsSeveralAlignments)
{
  // Arrange
  cardano_uplc_arena_t* arena = nullptr;
  ASSERT_EQ(cardano_uplc_arena_new(4096U, &arena), CARDANO_SUCCESS);

  const size_t aligns[] = { 1U, 2U, 4U, 8U, 16U, 32U, 64U };
  const size_t sizes[]  = { 1U, 3U, 7U, 5U, 9U, 13U, 17U };

  // Act + Assert
  for (size_t i = 0U; i < (sizeof(aligns) / sizeof(aligns[0])); ++i)
  {
    void* ptr = cardano_uplc_arena_alloc(arena, sizes[i], aligns[i]);

    EXPECT_THAT(ptr, testing::Not((void*)nullptr));
    EXPECT_TRUE(is_aligned(ptr, aligns[i]));
  }

  // Cleanup
  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_arena_alloc, defaultAlignmentIsMaxAlign)
{
  // Arrange
  cardano_uplc_arena_t* arena = nullptr;
  ASSERT_EQ(cardano_uplc_arena_new(4096U, &arena), CARDANO_SUCCESS);

  // Act
  (void)cardano_uplc_arena_alloc(arena, 1U, 1U);
  void* ptr = cardano_uplc_arena_alloc(arena, 8U, 0U);

  // Assert
  EXPECT_THAT(ptr, testing::Not((void*)nullptr));
  EXPECT_TRUE(is_aligned(ptr, sizeof(void*)));

  // Cleanup
  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_arena_alloc, distinctAllocationsDoNotOverlap)
{
  // Arrange
  cardano_uplc_arena_t* arena = nullptr;
  ASSERT_EQ(cardano_uplc_arena_new(4096U, &arena), CARDANO_SUCCESS);

  // Act
  uint8_t* a = reinterpret_cast<uint8_t*>(cardano_uplc_arena_alloc(arena, 16U, 1U));
  uint8_t* b = reinterpret_cast<uint8_t*>(cardano_uplc_arena_alloc(arena, 16U, 1U));

  // Assert
  ASSERT_THAT(a, testing::Not((uint8_t*)nullptr));
  ASSERT_THAT(b, testing::Not((uint8_t*)nullptr));
  EXPECT_GE(b, a + 16);

  // Cleanup
  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_arena_alloc, zeroSizedAllocationSucceeds)
{
  // Arrange
  cardano_uplc_arena_t* arena = nullptr;
  ASSERT_EQ(cardano_uplc_arena_new(1024U, &arena), CARDANO_SUCCESS);

  // Act
  void* ptr = cardano_uplc_arena_alloc(arena, 0U, 8U);

  // Assert
  EXPECT_THAT(ptr, testing::Not((void*)nullptr));

  // Cleanup
  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_arena_alloc, rollsOverToNewBlockWhenCurrentIsFull)
{
  // Arrange
  cardano_uplc_arena_t* arena = nullptr;
  ASSERT_EQ(cardano_uplc_arena_new(64U, &arena), CARDANO_SUCCESS);

  // Act
  for (size_t i = 0U; i < 32U; ++i)
  {
    void* ptr = cardano_uplc_arena_alloc(arena, 48U, 8U);
    EXPECT_THAT(ptr, testing::Not((void*)nullptr));
    EXPECT_TRUE(is_aligned(ptr, 8U));
  }

  // Assert
  EXPECT_GE(cardano_uplc_arena_bytes_used(arena), 32U * 48U);

  // Cleanup
  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_arena_alloc, oversizedAllocationGetsOwnBlock)
{
  // Arrange
  cardano_uplc_arena_t* arena = nullptr;
  ASSERT_EQ(cardano_uplc_arena_new(64U, &arena), CARDANO_SUCCESS);

  // Act
  void* small  = cardano_uplc_arena_alloc(arena, 8U, 8U);
  void* big    = cardano_uplc_arena_alloc(arena, 4096U, 16U);
  void* small2 = cardano_uplc_arena_alloc(arena, 8U, 8U);

  // Assert
  EXPECT_THAT(small, testing::Not((void*)nullptr));
  EXPECT_THAT(big, testing::Not((void*)nullptr));
  EXPECT_THAT(small2, testing::Not((void*)nullptr));
  EXPECT_TRUE(is_aligned(big, 16U));

  // Cleanup
  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_arena_alloc, returnsNullWhenBackingAllocatorFailsOnGrow)
{
  // Arrange
  cardano_uplc_arena_t* arena = nullptr;
  ASSERT_EQ(cardano_uplc_arena_new(64U, &arena), CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  void* ptr = cardano_uplc_arena_alloc(arena, 16U, 8U);

  // Assert
  EXPECT_EQ(ptr, (void*)nullptr);
  EXPECT_EQ(cardano_uplc_arena_bytes_used(arena), 0U);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_arena_alloc, enforcesByteCeiling)
{
  // Arrange
  cardano_uplc_arena_t* arena = nullptr;
  ASSERT_EQ(cardano_uplc_arena_new(1024U, &arena), CARDANO_SUCCESS);

  // Act
  void* ptr = cardano_uplc_arena_alloc(arena, SIZE_MAX, 1U);

  // Assert
  EXPECT_EQ(ptr, (void*)nullptr);

  // Cleanup
  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_arena_alloc, enforcesByteCeilingAfterPriorAllocation)
{
  // Arrange
  cardano_uplc_arena_t* arena = nullptr;
  ASSERT_EQ(cardano_uplc_arena_new(1024U * 1024U, &arena), CARDANO_SUCCESS);

  // Act
  void* first = cardano_uplc_arena_alloc(arena, 1024U, 8U);
  ASSERT_THAT(first, testing::Not((void*)nullptr));

  void* second = cardano_uplc_arena_alloc(arena, (size_t)600U * 1024U * 1024U, 8U);

  // Assert
  EXPECT_EQ(second, (void*)nullptr);

  // Cleanup
  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_arena_alloc, fastPathChargeRejectsAllocationThatCrossesCeiling)
{
  // Arrange
  cardano_uplc_arena_t* arena = nullptr;
  ASSERT_EQ(cardano_uplc_int_arena_new_with_ceiling(4096U, 256U, &arena), CARDANO_SUCCESS);

  // Act
  void* first  = cardano_uplc_arena_alloc(arena, 200U, 1U);
  void* second = cardano_uplc_arena_alloc(arena, 100U, 1U);

  // Assert
  EXPECT_THAT(first, testing::Not((void*)nullptr));
  EXPECT_EQ(second, (void*)nullptr);
  EXPECT_EQ(cardano_uplc_arena_bytes_used(arena), 200U);

  // Cleanup
  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_arena_alloc, oversizePathRejectsAllocationThatCrossesCeiling)
{
  // Arrange
  // An oversize request past the ceiling is rejected by the charge before
  // grow_arena ever calls the backing allocator, so injecting a malloc failure
  // is unnecessary: the count of backing allocations must stay at zero.
  cardano_uplc_arena_t* arena = nullptr;
  ASSERT_EQ(cardano_uplc_int_arena_new_with_ceiling(64U, 256U, &arena), CARDANO_SUCCESS);

  reset_allocators_run_count();

  // Act
  void* ptr = cardano_uplc_arena_alloc(arena, 4096U, 16U);

  // Assert
  EXPECT_EQ(ptr, (void*)nullptr);
  EXPECT_EQ(cardano_uplc_arena_bytes_used(arena), 0U);

  // Cleanup
  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_arena_alloc, rejectsBlockSizeThatOverflowsBlockHeader)
{
  // Arrange
  // A block_size near SIZE_MAX makes the block payload plus header overflow in
  // grow_arena. The ceiling is maxed so the small request clears the charge and
  // reaches the grow path; no large allocation is attempted.
  cardano_uplc_arena_t* arena = nullptr;
  ASSERT_EQ(cardano_uplc_int_arena_new_with_ceiling(SIZE_MAX, SIZE_MAX, &arena), CARDANO_SUCCESS);

  // Act
  void* ptr = cardano_uplc_arena_alloc(arena, 1U, 1U);

  // Assert
  EXPECT_EQ(ptr, (void*)nullptr);
  EXPECT_EQ(cardano_uplc_arena_bytes_used(arena), 0U);

  // Cleanup
  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_arena_alloc, rejectsSizeThatOverflowsAlignmentPadding)
{
  // Arrange
  cardano_uplc_arena_t* arena = nullptr;
  ASSERT_EQ(cardano_uplc_int_arena_new_with_ceiling(64U, 256U, &arena), CARDANO_SUCCESS);

  // Act
  void* ptr = cardano_uplc_arena_alloc(arena, SIZE_MAX, 16U);

  // Assert
  EXPECT_EQ(ptr, (void*)nullptr);
  EXPECT_EQ(cardano_uplc_arena_bytes_used(arena), 0U);

  // Cleanup
  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_arena_register_unref, returnsErrorWhenArenaIsNull)
{
  // Act
  size_t          dummy = 0U;
  cardano_error_t error = cardano_uplc_arena_register_unref(nullptr, &dummy, counting_unref);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);
}

TEST(cardano_uplc_arena_register_unref, returnsErrorWhenObjectIsNull)
{
  // Arrange
  cardano_uplc_arena_t* arena = nullptr;
  ASSERT_EQ(cardano_uplc_arena_new(1024U, &arena), CARDANO_SUCCESS);

  // Act
  cardano_error_t error = cardano_uplc_arena_register_unref(arena, nullptr, counting_unref);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_arena_register_unref, returnsErrorWhenUnrefIsNull)
{
  // Arrange
  cardano_uplc_arena_t* arena = nullptr;
  size_t                dummy = 0U;
  ASSERT_EQ(cardano_uplc_arena_new(1024U, &arena), CARDANO_SUCCESS);

  // Act
  cardano_error_t error = cardano_uplc_arena_register_unref(arena, &dummy, nullptr);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_POINTER_IS_NULL);

  // Cleanup
  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_arena_register_unref, returnsErrorWhenNodeAllocationFails)
{
  // Arrange
  cardano_uplc_arena_t* arena = nullptr;
  size_t                dummy = 0U;
  ASSERT_EQ(cardano_uplc_arena_new(1024U, &arena), CARDANO_SUCCESS);

  reset_allocators_run_count();
  cardano_set_allocators(fail_right_away_malloc, realloc, free);

  // Act
  cardano_error_t error = cardano_uplc_arena_register_unref(arena, &dummy, counting_unref);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_MEMORY_ALLOCATION_FAILED);
  EXPECT_EQ(cardano_uplc_arena_bytes_used(arena), 0U);

  // Cleanup
  cardano_set_allocators(malloc, realloc, free);
  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_arena_register_unref, returnsIllegalStateWhenNodeCrossesCeiling)
{
  // Arrange
  cardano_uplc_arena_t* arena = nullptr;
  size_t                dummy = 0U;
  ASSERT_EQ(cardano_uplc_int_arena_new_with_ceiling(1024U, 8U, &arena), CARDANO_SUCCESS);

  // Act
  cardano_error_t error = cardano_uplc_arena_register_unref(arena, &dummy, counting_unref);

  // Assert
  EXPECT_EQ(error, CARDANO_ERROR_ILLEGAL_STATE);
  EXPECT_EQ(cardano_uplc_arena_bytes_used(arena), 0U);

  // Cleanup
  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_arena_register_unref, runsCallbacksOnFree)
{
  // Arrange
  cardano_uplc_arena_t* arena = nullptr;
  size_t                a     = 0U;
  size_t                b     = 0U;
  ASSERT_EQ(cardano_uplc_arena_new(1024U, &arena), CARDANO_SUCCESS);

  g_counter_unref_calls = 0U;

  ASSERT_EQ(cardano_uplc_arena_register_unref(arena, &a, counting_unref), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_uplc_arena_register_unref(arena, &b, counting_unref), CARDANO_SUCCESS);

  // Act
  cardano_uplc_arena_free(&arena);

  // Assert
  EXPECT_EQ(g_counter_unref_calls, 2U);
  EXPECT_EQ(a, 1U);
  EXPECT_EQ(b, 1U);
}

TEST(cardano_uplc_arena_register_unref, balancesBigintRefcountOnFree)
{
  // Arrange
  cardano_uplc_arena_t* arena  = nullptr;
  cardano_bigint_t*     bigint = nullptr;
  ASSERT_EQ(cardano_uplc_arena_new(1024U, &arena), CARDANO_SUCCESS);
  ASSERT_EQ(cardano_bigint_from_int(42, &bigint), CARDANO_SUCCESS);

  cardano_bigint_ref(bigint);
  EXPECT_EQ(cardano_bigint_refcount(bigint), 2U);

  ASSERT_EQ(cardano_uplc_arena_register_unref(arena, reinterpret_cast<cardano_object_t*>(bigint), object_unref_adapter), CARDANO_SUCCESS);

  // Act
  cardano_uplc_arena_free(&arena);

  // Assert
  EXPECT_EQ(cardano_bigint_refcount(bigint), 1U);

  // Cleanup
  cardano_bigint_unref(&bigint);
}

TEST(cardano_uplc_arena_bytes_used, returnsZeroWhenArenaIsNull)
{
  // Assert
  EXPECT_EQ(cardano_uplc_arena_bytes_used(nullptr), 0U);
}

TEST(cardano_uplc_arena_bytes_used, accountsForAlignmentPadding)
{
  // Arrange
  cardano_uplc_arena_t* arena = nullptr;
  ASSERT_EQ(cardano_uplc_arena_new(4096U, &arena), CARDANO_SUCCESS);

  // Act
  uint8_t* first  = reinterpret_cast<uint8_t*>(cardano_uplc_arena_alloc(arena, 1U, 1U));
  uint8_t* second = reinterpret_cast<uint8_t*>(cardano_uplc_arena_alloc(arena, 8U, 64U));

  // Assert
  ASSERT_THAT(first, testing::Not((uint8_t*)nullptr));
  ASSERT_THAT(second, testing::Not((uint8_t*)nullptr));
  EXPECT_TRUE(is_aligned(second, 64U));
  EXPECT_EQ(cardano_uplc_arena_bytes_used(arena), static_cast<size_t>((second + 8) - first));

  // Cleanup
  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_arena_free, toleratesNullArguments)
{
  // Arrange
  cardano_uplc_arena_t* arena = nullptr;

  // Act + Assert
  cardano_uplc_arena_free(nullptr);
  cardano_uplc_arena_free(&arena);

  SUCCEED();
}

TEST(cardano_uplc_arena_reset, toleratesNullArgument)
{
  // Act + Assert
  cardano_uplc_arena_reset(nullptr);

  SUCCEED();
}

TEST(cardano_uplc_arena_reset, runsUnrefCallbacksAndClearsTheList)
{
  // Arrange
  cardano_uplc_arena_t* arena = nullptr;
  ASSERT_EQ(cardano_uplc_arena_new(1024U, &arena), CARDANO_SUCCESS);

  size_t marker         = 0U;
  g_counter_unref_calls = 0U;

  ASSERT_EQ(cardano_uplc_arena_register_unref(arena, &marker, counting_unref), CARDANO_SUCCESS);

  // Act
  cardano_uplc_arena_reset(arena);

  // Assert
  EXPECT_EQ(g_counter_unref_calls, 1U);
  EXPECT_EQ(marker, 1U);

  cardano_uplc_arena_reset(arena);
  EXPECT_EQ(g_counter_unref_calls, 1U);

  cardano_uplc_arena_free(&arena);
}

TEST(cardano_uplc_arena_reset, rewindsBytesAndReusesBlockMemory)
{
  // Arrange
  cardano_uplc_arena_t* arena = nullptr;
  ASSERT_EQ(cardano_uplc_arena_new(1024U, &arena), CARDANO_SUCCESS);

  uint8_t* first = static_cast<uint8_t*>(cardano_uplc_arena_alloc(arena, 64U, 8U));
  ASSERT_THAT(first, testing::Not((uint8_t*)nullptr));
  EXPECT_GT(cardano_uplc_arena_bytes_used(arena), 0U);

  // Act
  cardano_uplc_arena_reset(arena);

  // Assert
  EXPECT_EQ(cardano_uplc_arena_bytes_used(arena), 0U);

  uint8_t* second = static_cast<uint8_t*>(cardano_uplc_arena_alloc(arena, 64U, 8U));
  EXPECT_EQ(second, first);

  cardano_uplc_arena_free(&arena);
}
